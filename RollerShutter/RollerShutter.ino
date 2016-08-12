// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

#define MY_RF24_PA_LEVEL RF24_PA_HIGH
// uncomment if we want to manually assign an ID
//#define MY_NODE_ID 1 /

#include <Bounce2.h>
#include <MySensors.h>
#include <SPI.h>

#define BUTTON_UP_PIN  2  // Arduino Digital I/O pin number for button 
#define BUTTON_DOWN_PIN  3  // Arduino Digital I/O pin number for button 
#define BUTTON_STOP_PIN  4  // Arduino Digital I/O pin number for button 
#define RELAY_DIR_PIN  5  // Arduino Digital I/O pin number for relay 
#define RELAY_POWER_PIN  6  // Arduino Digital I/O pin number for relay 
#define RELAY_ON 1
#define RELAY_OFF 0
#define DIRECTION_DOWN 1
#define DIRECTION_UP 0
#define SKETCH_NAME "roller shuter"
#define SKETCH_VER "2.0"
#define CHILD_ID 1   // sensor Id of the sensor child
#define PRESENT_MESSAGE "sensor for roller shutter"
const int LEVELS = 100; //the number of levels
const float rollTime = 15.0; //the overall rolling time of the shutter, to be manualy chnage for every shutter
const bool IS_ACK = false; //is to acknowlage 

// debouncing parameters
int value = 0;
int oldValueUp = 0;
int oldValueDown = 0;
int oldValueStop = 0;

// shutter position parameters
float timeOneLevel = rollTime / LEVELS;
int requestedShutterLevel = 0;
int currentShutterLevel = 0;
unsigned long lastLevelTime = 0;
bool isMoving = false;
int directionUpDown;

Bounce debouncerUp = Bounce();
Bounce debouncerDown = Bounce();
Bounce debouncerStop = Bounce();

MyMessage msgUp(CHILD_ID, V_UP);
MyMessage msgDown(CHILD_ID, V_DOWN);
MyMessage msgStop(CHILD_ID, V_STOP);

void shuttersUp(void)
{
  Serial.println("Shutters going up.");
  digitalWrite(RELAY_DIR_PIN, RELAY_OFF);
  delay(100);
  digitalWrite(RELAY_POWER_PIN, RELAY_ON);
  directionUpDown = DIRECTION_UP;
  isMoving = true;
}

void shuttersDown(void)
{
  Serial.println("Shutters going down.");
  digitalWrite(RELAY_DIR_PIN, RELAY_ON);
  delay(100);
  digitalWrite(RELAY_POWER_PIN, RELAY_ON);
  directionUpDown = DIRECTION_DOWN;
  isMoving = true;
}

void shuttersHalt(void)
{
  Serial.println("Shutters halted.");
  digitalWrite(RELAY_POWER_PIN, RELAY_OFF);
  delay(100);
  digitalWrite(RELAY_DIR_PIN, RELAY_OFF);
  isMoving = false;
  requestedShutterLevel = currentShutterLevel;
  saveState(CHILD_ID, currentShutterLevel);
  Serial.print("saving level to: ");
  Serial.println(currentShutterLevel);
}

void changeShuttersLevel(int level){
	if (level < 0 || level > 100) {
		Serial.println("level is out of range calling InitShutters with:");
		Serial.println(level);
		InitShutters(level);
	} else {
		int dir = (level > currentShutterLevel) ? DIRECTION_DOWN : DIRECTION_UP;
		if (isMoving && dir != directionUpDown) {
			shuttersHalt();
		}
		Serial.println("setting requested level to:");
		Serial.println(level);
		requestedShutterLevel = level;
	}
}

bool InitShutters(int level)
{
  if (level < 0 || level > 100) 
  {
    Serial.println("Current level unsure, calibrating...");
    shuttersUp();
    delay((rollTime + timeOneLevel * LEVELS) * 1000);
    shuttersHalt();
    return true;
  }
  else 
  {
    Serial.print("No clibration needed. setting to: ");
    Serial.println(level);
    currentShutterLevel = level;
	  changeShuttersLevel(currentShutterLevel);
    return false;
  }
}

void receive(const MyMessage &message) {
  Serial.println("recieved incomming message");
  switch (message.type) {
    case V_UP:
      Serial.print("Incoming change for ID_S_COVER:");
      Serial.print(message.sensor);
      Serial.print(", New status: ");
      Serial.println("V_UP");
      changeShuttersLevel(0);
      Serial.println("Done shutterAction procedure");
      break;

    case V_DOWN:
      Serial.print("Incoming change for ID_S_COVER:");
      Serial.print(message.sensor);
      Serial.print(", New status: ");
      Serial.println("V_DOWN");
      changeShuttersLevel(100);
      Serial.println("Done shutterAction procedure");
      break;

    case V_STOP:
      Serial.print("Incoming change for ID_S_COVER:");
      Serial.print(message.sensor);
      Serial.print(", New status: ");
      Serial.println("V_STOP");
      shuttersHalt();
      Serial.println("Done shutterAction procedure");
      break;
  
	case V_PERCENTAGE:
	  Serial.print("Incoming change for ID_S_COVER:");
      Serial.print(message.sensor);
      Serial.print(", New status: ");
      Serial.println("V_PERCENTAGE");
      changeShuttersLevel(message.getInt());
	  //InitShutters(message.getInt());//send value < 0 or > 100 to calibrate
      Serial.println("Done shutterAction procedure");
      break;
  }
  Serial.println("exiting incoming message");
  return;
}

void presentation()  
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo(SKETCH_NAME, SKETCH_VER);
	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID, S_COVER, PRESENT_MESSAGE, IS_ACK);
}

void setup(void)
{
   Serial.begin(115200);
   Serial.println("StartUP");

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
  digitalWrite(RELAY_DIR_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_DIR_PIN, OUTPUT);

  // Make sure relays are off when starting up
  digitalWrite(RELAY_POWER_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_POWER_PIN, OUTPUT);

  InitShutters(loadState(CHILD_ID));
}

void loop(void)
{
  debouncerUp.update();
  value = debouncerUp.read();
  if (value == 0 && value != oldValueUp) {
    changeShuttersLevel(0);
    send(msgUp, IS_ACK);
  }
  oldValueUp = value;

  debouncerDown.update();
  value = debouncerDown.read();
  if (value == 0 && value != oldValueDown) {
    changeShuttersLevel(100);
    send(msgDown, IS_ACK);
  }
  oldValueDown = value;

  debouncerStop.update();
  value = debouncerStop.read();
  if (value == 0 && value != oldValueStop) {
    shuttersHalt();
    send(msgStop, IS_ACK);
  }
  oldValueStop = value;
  
  
  if (isMoving) 
  {
    //Serial.println("1");
    unsigned long _now = millis();
    if (_now - lastLevelTime >= timeOneLevel * 1000) 
    {
      //Serial.println("2");
      if (directionUpDown == DIRECTION_DOWN) 
      {
        //Serial.println("3");
        currentShutterLevel += 1;
      } 
      else 
      {
        //Serial.println("4");
        currentShutterLevel -= 1;
      }
      lastLevelTime = millis();
    }
    
    if (currentShutterLevel == requestedShutterLevel)
    {
      //Serial.println("5");   
      shuttersHalt();
    }
  }
  else if (requestedShutterLevel != currentShutterLevel) 
  {
    //Serial.println("6");
    if (requestedShutterLevel < currentShutterLevel)
    {
      //Serial.println("7");
      shuttersUp();
    } 
    else 
    {
     // Serial.println("8");
      shuttersDown();
    }
    lastLevelTime = millis();
  }
}
