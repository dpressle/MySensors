// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

// #define MY_RF24_PA_LEVEL RF24_PA_HIGH
// uncomment if we want to manually assign an ID
//#define MY_NODE_ID 1 /

#include <MySensors.h>
#include <SPI.h>
#include <Tadiran.h>
#include <IRremote.h>

#define SKETCH_NAME "AC Control"
#define SKETCH_VER "1.0"
#define CHILD_ID 1   // sensor Id of the sensor child
#define PRESENT_MESSAGE "sensor for AC control"

bool codeReady = false;
const bool IS_ACK = false; //is to acknowlage 
IRsend irsend;
Tadiran tadiran(MODE_auto, FAN_auto, 26, STATE_off);

// MyMessage msgUp(CHILD_ID, V_UP);

void receive(const MyMessage &message) {
  Serial.println("recieved incomming message");
  switch (message.type) {
    case V_VAR1:
	  Serial.print("Incoming change for CHILD_ID:");
      Serial.print(message.sensor);
      Serial.print(", New status: V_VAR1, with payload: ");
      Serial.print(message.getInt());
	  tadiran.setTemeprature(message.getInt());
	  codeReady = true;
	  break;
	case V_VAR2:
	  Serial.print("Incoming change for CHILD_ID:");
      Serial.print(message.sensor);
      Serial.print(", New status: V_VAR2, with payload: ");
      Serial.print(message.getInt());
	  tadiran.setMode(message.getInt());
	  codeReady = true;
	  break;
	case V_VAR3:
	  Serial.print("Incoming change for CHILD_ID:");
      Serial.print(message.sensor);
      Serial.print(", New status: V_VAR3, with payload: ");
      Serial.print(message.getInt());
	  codeReady = true;
	  break;
	case V_VAR4:
	  Serial.print("Incoming change for CHILD_ID:");
      Serial.print(message.sensor);
      Serial.print(", New status: V_VAR4, with payload: ");
      Serial.print(message.getBool());
	  if (message.getBool()) {
	    tadiran.setState(STATE_on);
	  } else {
	    tadiran.setState(STATE_off);
	  }
	  codeReady = true;
	  break;
   }
   Serial.println("exiting incoming message");
}

void presentation()  
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo(SKETCH_NAME, SKETCH_VER);
	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID, S_CUSTOM, PRESENT_MESSAGE, IS_ACK);
}

void setup(void)
{
   Serial.begin(115200);
   Serial.println("StartUP");
}

void loop(void) {
  if (codeReady) {
    Serial.println("Sending code...");
    irsend.sendRaw(tadiran.codes, TADIRAN_BUFFER_SIZE, 38);
	codeReady = false;
  }
}
