/*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*/

// Enable debug prints to serial monitor
//#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <Tadiran.h>
#include <IRremote.h>
#include <MySensors.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>

IRsend irsend;
Tadiran hvac(MODE_cold, FAN_auto, 26, STATE_off);
bool initial_state_sent = false;

#define ONE_WIRE_BUS 2 // Pin where dallase sensor is connected 
//#define MAX_ATTACHED_DS18B20 1
unsigned long SLEEP_TIME = 60000; // Sleep time between reads (in milliseconds) 1 minute
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
float lastTemperature;

#define CHILD_ID_HVAC 0  // childId
#define CHILD_ID_MODE 1
#define CHILD_ID_SPEED 2
#define CHILD_ID_SET_POINT 3

MyMessage msgHVACSetPointC(CHILD_ID_HVAC, V_HVAC_SETPOINT_COOL);
MyMessage msgHVACSpeed(CHILD_ID_HVAC, V_HVAC_SPEED);
MyMessage msgHVACFlowState(CHILD_ID_HVAC, V_HVAC_FLOW_STATE);
MyMessage msgHVACTemperature(CHILD_ID_HVAC, V_TEMP);

/*
* Include all the other Necessary code here.
* The example code is limited to message exchange for mysensors
* with the controller (ha).
*/

void sendState() {
  if (hvac.getState() == STATE_off) send(msgHVACFlowState.set("Off"));
  else if (hvac.getMode() == MODE_cold) send(msgHVACFlowState.set("CoolOn"));
  else send(msgHVACFlowState.set("HeatOn"));
  wait(50);
  if (hvac.getFan() == FAN_auto) send(msgHVACSpeed.set("Auto"));
  else if (hvac.getFan() == FAN_1) send(msgHVACSpeed.set("Min"));
  else if(hvac.getFan() == FAN_2) send(msgHVACSpeed.set("Normal"));
  else if(hvac.getFan() == FAN_3) send(msgHVACSpeed.set("Max"));
  wait(50);
  send(msgHVACSetPointC.set(hvac.getTemeprature()));
}

void processHVAC(){
  #ifdef MY_DEBUG
  Serial.println("Sending code...");
  #endif
  irsend.sendRaw(hvac.codes, TADIRAN_BUFFER_SIZE, 38);
}

void before() {
  sensors.begin();
}

void presentation()  
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("HVAC", "1.0");
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HVAC, S_HVAC, "Thermostat");
}

void setup() {
  sensors.setWaitForConversion(false);
  int state = loadState(CHILD_ID_HVAC);
  #ifdef MY_DEBUG
  Serial.println("state is: ");
  Serial.println(state);
  #endif
  if (state != 0Xff) {
    hvac.setState(state);
  }
  state = loadState(CHILD_ID_MODE);
  if (state != 0Xff) {
    hvac.setMode(state);
  }
  state = loadState(CHILD_ID_SPEED);
  if (state != 0Xff) {
    hvac.setFan(state);
  }
  state = loadState(CHILD_ID_SET_POINT);
  if (state != 0Xff) {
    hvac.setTemeprature(state);
  }
}

void loop() {
 if (!initial_state_sent) {
    #ifdef MY_DEBUG
    Serial.println("Sending initial value");
    #endif
    sendState();
    initial_state_sent = true;
  }
   // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  wait(conversionTime);

  // Read temperatures and send them to controller 
  //for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {

    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getConfig().isMetric?sensors.getTempCByIndex(0):sensors.getTempFByIndex(0)) * 10.)) / 10.;
    //float temperature = 22.1;
    #ifdef MY_DEBUG
    Serial.println(temperature);
    #endif
    // Only send data if temperature has changed and no error
    if (lastTemperature != temperature && temperature != -127.00 && temperature != 85.00) {
      // Send in the new temperature
      send(msgHVACTemperature.set(temperature,1));
      // Save new temperatures for next compare
      lastTemperature = temperature;
    }
 // }
  wait(SLEEP_TIME);
}

void receive(const MyMessage &message) {
  String recvData = message.data;
  recvData.trim();
  switch (message.type) {
    case V_HVAC_FLOW_STATE:
      if(recvData.equalsIgnoreCase("coolon")){
        hvac.setState(STATE_on);
        hvac.setMode(MODE_cold);
        processHVAC();
        saveState(CHILD_ID_HVAC, STATE_on);
        saveState(CHILD_ID_MODE, MODE_cold);
		sendState();
      } else if(recvData.equalsIgnoreCase("heaton")){
        hvac.setState(STATE_on);
        hvac.setMode(MODE_heat);
        processHVAC();
        saveState(CHILD_ID_HVAC, STATE_on);
        saveState(CHILD_ID_MODE, MODE_heat);
		sendState();
       } else if(recvData.equalsIgnoreCase("off") && ( hvac.getState() == STATE_on ) ){
        hvac.setState(STATE_off);
        processHVAC();
        saveState(CHILD_ID_HVAC, STATE_off);
		sendState();
       }
      break;
    case V_HVAC_SPEED:
      if (recvData.equalsIgnoreCase("auto")) hvac.setFan(FAN_auto);
      else if (recvData.equalsIgnoreCase("min")) hvac.setFan(FAN_1);
      else if (recvData.equalsIgnoreCase("normal")) hvac.setFan(FAN_2);
      else if (recvData.equalsIgnoreCase("max")) hvac.setFan(FAN_3);
	  if (hvac.getState() == STATE_on)
		processHVAC();
      saveState(CHILD_ID_SPEED, hvac.getFan());
	  sendState();
      break;
    case V_HVAC_SETPOINT_COOL:
      int target_temp = message.getInt();
      hvac.setTemeprature(target_temp);
	  if (hvac.getState() == STATE_on)
		processHVAC();
      saveState(CHILD_ID_SET_POINT, hvac.getTemeprature());
	  sendState();
      break;
   }
}
