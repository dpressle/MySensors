/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik Ekblad
 * 
 * DESCRIPTION
 * Example sketch for a "light switch" where you can control light or something 
 * else from both HA controller and a local physical button 
 * (connected between digital pin 3 and GND).
 * This node also works as a repeader for other nodes
 * http://www.mysensors.org/build/relay
 */ 

// Enable debug prints to serial monitor
//#define MY_DEBUG 
//#define MY_NODE_ID 6
// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enabled repeater feature for this node
//#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensors.h>
#include <Bounce2.h>

#define RELAY_PIN 2  // Arduino Digital I/O pin number for relay 
#define RELAY_PIN1 3  // Arduino Digital I/O pin number for relay 
#define BUTTON_PIN 5  // Arduino Digital I/O pin number for button 
#define LED_PIN 6  // Arduino Digital I/O pin number for led 

#define CHILD_ID 0   // Id of the sensor child
#define RELAY_ON 1
#define RELAY_OFF 0

Bounce debouncer = Bounce(); 
int oldValue=0;
int state;
bool initial_state_sent;

MyMessage msg(CHILD_ID, V_STATUS);

void changeState(int newState) {
  state = newState;
  digitalWrite(RELAY_PIN, state);
  digitalWrite(RELAY_PIN1, state);
  digitalWrite(LED_PIN, state);
  saveState(CHILD_ID, state);
  send(msg.set(state));
}

void before() {
  // Setup the button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN, HIGH);

  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);

  // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN, OUTPUT);
  // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN1, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN1, OUTPUT);
  
  digitalWrite(LED_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(LED_PIN, OUTPUT);
}

void setup()  
{  
  // Set relay to last known state (using eeprom storage) 
  int newState = loadState(CHILD_ID);
  if (newState == 0Xff) {//if eeprom is empty
    newState = RELAY_OFF;
  }
  changeState(newState);
}

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("RelayButton", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID, S_BINARY);
}

/*
*  Example on how to asynchronously check for new messages from gw
*/
void loop() 
{
   if (!initial_state_sent) {
    Serial.println("Sending initial value");
    send(msg.set(state ? false : true));
    initial_state_sent = true;
  }
  debouncer.update();
  // Get the update value
  int value = debouncer.read();
  if (value != oldValue && value==0) {
	  //toggle switch state
	  changeState(state ? RELAY_OFF : RELAY_ON);
      //digitalWrite(RELAY_PIN, state ? RELAY_OFF : RELAY_ON);
      //send(msg.set(state?false:true)); // Send new state and request ack back
  }
  oldValue = value;
} 

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
  }

  if (message.type == V_LIGHT) {
     // Change relay state
     state = message.getBool();
     changeState(state ? RELAY_ON : RELAY_OFF);
     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}
