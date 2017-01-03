// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24

//#define MY_RF24_PA_LEVEL RF24_PA_LOW

//#define MY_REPEATER_FEATURE

// uncomment if we want to manually assign an ID
//#define MY_NODE_ID 1 /

#include <Bounce2.h>
#include <MySensors.h>
#include <SPI.h>

#define BUTTON_UP_PIN  2  // Arduino Digital I/O pin number for up button
#define BUTTON_DOWN_PIN  3  // Arduino Digital I/O pin number for down button
#define BUTTON_STOP_PIN  4  // Arduino Digital I/O pin number for stop button
#define RELAY_DIR_PIN  5  // Arduino Digital I/O pin number for direction relay
#define RELAY_POWER_PIN  6  // Arduino Digital I/O pin number for power relay
#define RELAY_ON 1
#define RELAY_OFF 0
#define DIRECTION_DOWN 1
#define DIRECTION_UP 0
#define SKETCH_NAME "roller shutter"
#define SKETCH_VER "2.1"
#define CHILD_ID 1   // sensor Id of the sensor child
//#define CHILD_ID_CALIBRATE 2   // sensor Id of the sensor child to calibrate
#define CHILD_ID_SET 2   // sensor Id of the sensor child to init the roll time
#define PRESENT_MESSAGE "sensor for roller shutter"
const int LEVELS = 100; //the number of levels
float rollTime = 15.0; //the overall rolling time of the shutter
const bool IS_ACK = false; //is to acknowlage

// debouncing parameters
int value = 0;
int oldValueUp = 0;
int oldValueDown = 0;
int oldValueStop = 0;
//static unsigned long last_interrupt_time_up = 0;
//static unsigned long last_interrupt_time_down = 0;
//static unsigned long debounce_time = 200;

Bounce debouncerUp = Bounce();
Bounce debouncerDown = Bounce();
Bounce debouncerStop = Bounce();

// shutter position parameters
float timeOneLevel = rollTime / LEVELS;
int requestedShutterLevel = 0;
int currentShutterLevel = 0;
unsigned long lastLevelTime = 0;
bool isMoving = false;
int directionUpDown;

MyMessage msgUp(CHILD_ID, V_UP);
MyMessage msgDown(CHILD_ID, V_DOWN);
MyMessage msgStop(CHILD_ID, V_STOP);
MyMessage msgPercentage(CHILD_ID, V_PERCENTAGE);

void print_debug(String message) {
  #ifdef MY_DEBUG
    Serial.println(message);
  #endif
}

void shuttersUp(void) {
  print_debug("Shutters going up.");
  directionUpDown = DIRECTION_UP;
  isMoving = true;
  digitalWrite(RELAY_DIR_PIN, RELAY_OFF);
  delay(20);
  digitalWrite(RELAY_POWER_PIN, RELAY_ON);
}

void shuttersDown(void) {
  print_debug("Shutters going down.");
  directionUpDown = DIRECTION_DOWN;
  isMoving = true;
  digitalWrite(RELAY_DIR_PIN, RELAY_ON);
  delay(20);
  digitalWrite(RELAY_POWER_PIN, RELAY_ON);

}

void shuttersHalt(void) {
  isMoving = false;
  requestedShutterLevel = currentShutterLevel;
  digitalWrite(RELAY_POWER_PIN, RELAY_OFF);
  delay(20);
  digitalWrite(RELAY_DIR_PIN, RELAY_OFF);
  print_debug("Shutters halted.");
  saveState(CHILD_ID, currentShutterLevel);
  print_debug("saving level to: ");
  print_debug(String(currentShutterLevel));
}

void changeShuttersLevel(int level) {
	if (level < 0 || level > 100) {
		print_debug("level is out of range calling InitShutters: ");
		print_debug(String(level));
		InitShutters();
		level = 0;
	} else {
		int dir = (level > currentShutterLevel) ? DIRECTION_DOWN : DIRECTION_UP;
		if (isMoving && dir != directionUpDown) {
			shuttersHalt();
		}
		print_debug("setting requested level to:");
		print_debug(String(level));
		requestedShutterLevel = level;
	}
}

void InitShutters() {
  // if (level < 0 || level > 100)
  // {
    //print_debug("Current level unsure, calibrating...");
    print_debug("Init Shutters");
    shuttersUp();
    print_debug("delaying for: ");
    print_debug(String((rollTime + timeOneLevel * LEVELS) * 1000));
    delay((rollTime + timeOneLevel * LEVELS) * 1000);
    print_debug("ended delay rolltime");
	  currentShutterLevel = 0;
	  requestedShutterLevel = currentShutterLevel;
    //return true;
  // }
  // else
  // {
    // print_debug("No clibration needed. setting to: ");
    // print_debug(level);
    // currentShutterLevel = level;
	  // changeShuttersLevel(currentShutterLevel);
    // return false;
  // }
}

void receive(const MyMessage &message) {
  print_debug("recieved incomming message");
  print_debug("Recieved message for sensor: ");
  print_debug(String(message.sensor));
  switch (message.sensor) {
    case CHILD_ID:
      switch (message.type) {
        case V_UP:
          print_debug(", New status: V_UP");
          changeShuttersLevel(0);
          print_debug("Done shutterAction procedure");
          break;

        case V_DOWN:
          print_debug(", New status: V_DOWN");
          changeShuttersLevel(100);
          print_debug("Done shutterAction procedure");
          break;

        case V_STOP:
          print_debug(", New status: V_STOP");
          shuttersHalt();
          print_debug("Done shutterAction procedure");
          break;

        case V_PERCENTAGE:
          print_debug(", New status: V_PERCENTAGE");
          changeShuttersLevel(message.getInt());
      	  //InitShutters(message.getInt());//send value < 0 or > 100 to calibrate
          print_debug("Done shutterAction procedure");
          break;
      }
    case CHILD_ID_SET:
     switch (message.type) {
       case V_VAR1:
         print_debug(", New status: V_VAR1, with payload: ");
         rollTime = message.getFloat();
         print_debug("rolltime value: ");
         print_debug(String(rollTime));
         saveState(CHILD_ID_SET, rollTime);
         break;
        case V_VAR2:
         print_debug(", New status: ");
         print_debug("V_VAR2, with payload: ");
         print_debug(String(message.getInt()));
         InitShutters();
         break;
     }
  }
  print_debug("exiting incoming message");
  return;
}

void presentation() {
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo(SKETCH_NAME, SKETCH_VER);
	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID, S_COVER, PRESENT_MESSAGE, IS_ACK);
  present(CHILD_ID_SET, S_CUSTOM, PRESENT_MESSAGE, IS_ACK);
}

void before() {
  
   // Setup the button
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  // Activate internal pull-up
  digitalWrite(BUTTON_UP_PIN, HIGH);
//  attachInterrupt(digitalPinToInterrupt(BUTTON_UP_PIN), upButtonPress, FALLING);
  
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  // Activate internal pull-up
  digitalWrite(BUTTON_DOWN_PIN, HIGH);
//  attachInterrupt(digitalPinToInterrupt(BUTTON_DOWN_PIN), downButtonPress, FALLING);
  
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

  
}

void setup(void) {
	Serial.begin(115200);
	Serial.println("StartUP");
	//set up roll time if the saved value is not 255
  print_debug("getting rolltime from eeprom: ");
  int tmpRollTime = loadState(CHILD_ID_SET);
  if (tmpRollTime < 255) {
    rollTime = tmpRollTime;
  }
  print_debug(String(rollTime));
  
  print_debug("getting state from eeprom: ");
  int state = loadState(CHILD_ID);
  print_debug(String(state));
  changeShuttersLevel(state);  
}

void loop(void) {
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
    //print_debug("1");
    unsigned long _now = millis();
    if (_now - lastLevelTime >= timeOneLevel * 1000)
    {
      //print_debug("2");
      if (directionUpDown == DIRECTION_DOWN)
      {
        //print_debug("3");
        currentShutterLevel += 1;
      }
      else
      {
        //print_debug("4");
        currentShutterLevel -= 1;
      }
      lastLevelTime = millis();
    }

    if (currentShutterLevel == requestedShutterLevel)
    {
      //print_debug("5");
      shuttersHalt();
    }
  }
  else if (requestedShutterLevel != currentShutterLevel)
  {
    //print_debug("6");
    if (requestedShutterLevel < currentShutterLevel)
    {
      //print_debug("7");
      shuttersUp();
    }
    else
    {
     // print_debug("8");
      shuttersDown();
    }
    lastLevelTime = millis();
  }
}

//void upButtonPress() {
//  unsigned long interrupt_time = millis();
//  // If interrupts come faster than 200ms, assume it's a bounce and ignore
//  if (interrupt_time - last_interrupt_time_up > debounce_time) 
//  {
//    changeShuttersLevel(0);
//    send(msgUp, IS_ACK);
//  }
//  last_interrupt_time_up = interrupt_time;
//}
//
//void downButtonPress() {
//  unsigned long interrupt_time = millis();
//  // If interrupts come faster than 200ms, assume it's a bounce and ignore
//  if (interrupt_time - last_interrupt_time_down > debounce_time) 
//  {
//    changeShuttersLevel(100);
//    send(msgDown, IS_ACK);
//   }
//  last_interrupt_time_down = interrupt_time;
//}
