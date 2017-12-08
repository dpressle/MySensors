// Enable debug prints to serial monitor
#define MY_DEBUG 
//#define MY_NODE_ID 6
// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69
// #define MY_RF24_CE_PIN 7
// #define MY_RF24_CS_PIN 8
// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensors.h>
// #include <Bounce2.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define RELAY_PIN1 3  // Arduino Digital I/O pin number for relay 
#define RELAY_PIN2 4  // Arduino Digital I/O pin number for relay 

#define ONE_WIRE_BUS 5 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16

//#define CHILD_ID_TEMP 0   // Id of the temp sensor child
#define CHILD_ID_SWITCH 0   // Id of the switch sensor child

#define RELAY_ON 1
#define RELAY_OFF 0

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;
int state;
bool initial_state_sent;

MyMessage switchMsg(CHILD_ID_SWITCH, V_STATUS);
MyMessage tempMsg(1,V_TEMP);

void changeState(int newState)
{
  saveState(CHILD_ID_SWITCH, newState);
  digitalWrite(RELAY_PIN1, newState);
  digitalWrite(RELAY_PIN2, newState);
  send(switchMsg.set(newState));
}

void before()
{
  // Startup up the OneWire library
  sensors.begin();

  // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN1, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN1, OUTPUT);
  // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN2, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN2, OUTPUT);
}

void setup()  
{ 
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);

  int newState = loadState(CHILD_ID_SWITCH);
  if (newState == 0Xff) {//if eeprom is empty
    newState = RELAY_OFF;
  }
  changeState(newState);
}

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("BoilerSwitch", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_SWITCH, S_BINARY);
  
  numSensors = sensors.getDeviceCount();
  Serial.println("numSensors: ");
  Serial.println(numSensors);
  
  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(i+1, S_TEMP, "Temperature sensor " + (i+1));
  }
}

void loop()     
{     
    if (!initial_state_sent) {
        Serial.println("Sending initial value");
        send(switchMsg.set(loadState(CHILD_ID_SWITCH) ? true : false));
        initial_state_sent = true;
    }
  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  sleep(conversionTime);

  // Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {

    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
#ifdef MY_DEBUG
    Serial.println(temperature);
#endif
    // Only send data if temperature has changed and no error
    if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) {
      // Send in the new temperature
      send(tempMsg.setSensor(i + 1).set(temperature,1));
      // Save new temperatures for next compare
      lastTemperature[i]=temperature;
    }
  }
  wait(60000);//wait for 1 minute before checking again
}

void receive(const MyMessage &message)
{
  // We only expect one type of message from controller. But we better check anyway.
  if (message.isAck())
  {
#ifdef MY_DEBUG
    Serial.println("This is an ack from gateway");
#endif
  }

  if (message.type == V_LIGHT)
  {
#ifdef MY_DEBUG
    Serial.print("Incoming change for sensor:");
    Serial.print(message.sensor);
    Serial.print(", New status: ");
    Serial.println(message.getBool());
#endif
    // chnage the state of the related sensor
    changeState(message.getBool() ? RELAY_ON : RELAY_OFF);
  }
}
