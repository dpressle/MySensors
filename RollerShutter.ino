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
 * Version 1.0
 *
 * DESCRIPTION
 */

#include <MySensor.h>
#include <SPI.h>
#include <Bounce2.h>

#define BUTTON_UP_PIN  2  // Arduino Digital I/O pin number for button 
#define BUTTON_DOWN_PIN  3  // Arduino Digital I/O pin number for button 
#define BUTTON_STOP_PIN  4  // Arduino Digital I/O pin number for button 
#define RELAY_PIN  5  // Arduino Digital I/O pin number for relay 
#define RELAY_POWER_PIN  6  // Arduino Digital I/O pin number for relay 

#define NODE_ID 2 //set the node id else put AUTO to auto assign by controller 
#define CHILD_ID 1   // Id of the sensor child

#define RELAY_ON 1
#define RELAY_OFF 0
#define UP 0
#define DOWN 1
#define STOP 2
#define SKETCH_NAME "roller shuter"
#define SKETCH_VER "1.0"

const bool IS_ACK = true;

Bounce debouncerUp = Bounce();
Bounce debouncerDown = Bounce();
Bounce debouncerStop = Bounce();

MySensor gw;
MyMessage msgUp(CHILD_ID, V_UP);
MyMessage msgDown(CHILD_ID, V_DOWN);
MyMessage msgStop(CHILD_ID, V_STOP);

int value = 0;
int oldValueUp = 0;
int oldValueDown = 0;
int oldValueStop = 0;

void setup()
{
  Serial.begin(115200);

  gw.begin(incomingMessage, NODE_ID, IS_ACK);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo(SKETCH_NAME, SKETCH_VER);

  // Setup the button
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  // Activate internal pull-up
  digitalWrite(BUTTON_UP_PIN, HIGH);

  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  // Activate internal pull-up
  digitalWrite(BUTTON_DOWN_PIN, HIGH);

  pinMode(BUTTON_STOP_PIN, INPUT_PULLUP);
  // Activate internal pull-up
  digitalWrite(BUTTON_STOP_PIN, HIGH);

  // After setting up the button, setup debouncer
  debouncerUp.attach(BUTTON_UP_PIN);
  debouncerUp.interval(5);
  // After setting up the button, setup debouncer
  debouncerDown.attach(BUTTON_DOWN_PIN);
  debouncerDown.interval(5);
  // After setting up the button, setup debouncer
  debouncerStop.attach(BUTTON_STOP_PIN);
  debouncerStop.interval(5);

  // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN, OUTPUT);

  // Make sure relays are off when starting up
  digitalWrite(RELAY_POWER_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_POWER_PIN, OUTPUT);

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID, S_COVER);

  // Set shutter to last known state (using eeprom storage)
  shutterAction(gw.loadState(CHILD_ID));
}


/*
*  Example on how to asynchronously check for new messages from gw
*/
void loop()
{
  gw.process();

  debouncerUp.update();
  // Get the update value
  value = debouncerUp.read();
  if (value == 0 && value != oldValueUp) {
    shutterAction(UP);
	gw.send(msgUp);
  }
  oldValueUp = value;

  debouncerDown.update();
  value = debouncerDown.read();
  if (value == 0 && value != oldValueDown) {
    shutterAction(DOWN);
	gw.send(msgDown);
  }
  oldValueDown = value;

  debouncerStop.update();
  value = debouncerStop.read();
  if (value == 0 && value != oldValueStop) {
    shutterAction(STOP);
	gw.send(msgStop);
  }
  oldValueStop = value;
}

void incomingMessage(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  Serial.println("recieved incomming message");
  switch (message.type) {
    case V_UP:
      Serial.print("Incoming change for ID_S_COVER:");
      Serial.print(message.sensor);
      Serial.print(", New status: ");
      Serial.println("V_UP");
      shutterAction(UP);
	  gw.saveState(CHILD_ID, UP);
      Serial.print("Done shutterAction procedure");
      break;

    case V_DOWN:
      Serial.print("Incoming change for ID_S_COVER:");
      Serial.print(message.sensor);
      Serial.print(", New status: ");
      Serial.println("V_DOWN");
      shutterAction(DOWN);
	  gw.saveState(CHILD_ID, DOWN);
      Serial.print("Done shutterAction procedure");
      break;

    case V_STOP:
      Serial.print("Incoming change for ID_S_COVER:");
      Serial.print(message.sensor);
      Serial.print(", New status: ");
      Serial.println("V_STOP");
      shutterAction(STOP);
	  gw.saveState(CHILD_ID, STOP);
      Serial.print("Done shutterAction procedure");
      break;
  }
  Serial.print("exiting incoming message");
  return;
}

void shutterAction(int actionVal) {

  Serial.print("Shutter is : ");
  Serial.println(actionVal);
  switch (actionVal) {
    case UP:
      Serial.println("Opening");
      motorUp();
      //gw.process();
      //actionVal = gw.loadState(CHILD_ID);
      //gw.send(msgUp);
      break;
    case DOWN:
      Serial.println("Closing");
      motorDown();
      //gw.process();
      //actionVal = gw.loadState(CHILD_ID);
      //gw.send(msgDown);
      break;
    case STOP:
      Serial.println("Idle");
      motorStop();
      //gw.process();
      //actionVal = gw.loadState(CHILD_ID);
		//gw.send(msgStop);
      break;
  }
  return;

}

void motorUp() {
  motorStop();
  delay(100);
  digitalWrite(RELAY_PIN, RELAY_ON);
  delay(100);
  motorStart();
  delay(100);
}

void motorDown() {
  motorStop();
  delay(100);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  delay(100);
  motorStart();
  delay(100);
}

void motorStop() {
  digitalWrite(RELAY_POWER_PIN, RELAY_OFF);
  delay(100);
}

void motorStart() {
  digitalWrite(RELAY_POWER_PIN, RELAY_ON);
  delay(100);
}