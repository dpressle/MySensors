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

#define BUTTON_UP_PIN 3  // Arduino Digital I/O pin number for up button
#define BUTTON_DOWN_PIN 4  // Arduino Digital I/O pin number for down button
#define BUTTON_STOP_PIN 5  // Arduino Digital I/O pin number for stop button
#define RELAY_DIR_PIN 6  // Arduino Digital I/O pin number for direction relay
#define RELAY_POWER_PIN 7  // Arduino Digital I/O pin number for power relay
#define RELAY_ON 1
#define RELAY_OFF 0
#define RELAY_DOWN 1
#define RELAY_UP 0
#define DIRECTION_DOWN 1
#define DIRECTION_UP 0
#define SKETCH_NAME "Cover"
#define SKETCH_VER "2.0"
#define CHILD_ID_COVER 0   // sensor Id of the sensor child
#define STATE_UP 100 // 100 is open - up
#define STATE_DOWN 0 // 0 is closed - down
//#define CHILD_ID_CALIBRATE 1   // sensor Id of the sensor child to calibrate
#define CHILD_ID_SET 1   // sensor Id of the sensor child to init the roll time
#define PRESENT_MESSAGE "Cover sensor for hass"
const int LEVELS = 100; //the number of levels
float rollTime = 20.0; //the overall rolling time of the shutter
const bool IS_ACK = false; //is to acknowlage
static bool initial_state_sent = false;//for hass we need at list one state send at begining

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

enum CoverState {
  STOP,
  UP, // Window covering. Up.
  DOWN, // Window covering. Down.
};

static int coverState = STOP;

MyMessage msgUp(CHILD_ID_COVER, V_UP);
MyMessage msgDown(CHILD_ID_COVER, V_DOWN);
MyMessage msgStop(CHILD_ID_COVER, V_STOP);
MyMessage msgPercentage(CHILD_ID_COVER, V_PERCENTAGE);
//MyMessage msgCode(CHILD_ID_SET, V_IR_SEND);

void sendState() {
  // Send current state and status to gateway.
  send(msgUp.set(coverState == UP));
  send(msgDown.set(coverState == DOWN));
  send(msgStop.set(coverState == STOP));
  send(msgPercentage.set(currentShutterLevel));
}

void shuttersUp(void) {
#ifdef MY_DEBUG
  Serial.println("Shutters going up");
#endif
  if (digitalRead(RELAY_POWER_PIN) == RELAY_ON) {
    digitalWrite(RELAY_POWER_PIN, RELAY_OFF);
    delay(20);
  }
  digitalWrite(RELAY_DIR_PIN, RELAY_UP);
  delay(20);
  digitalWrite(RELAY_POWER_PIN, RELAY_ON);

  directionUpDown = DIRECTION_UP;
  isMoving = true;
  coverState = UP;
  sendState();
}

void shuttersDown(void) {
#ifdef MY_DEBUG
  Serial.println("Shutters going down");
#endif
  if (digitalRead(RELAY_POWER_PIN) == RELAY_ON) {
    digitalWrite(RELAY_POWER_PIN, RELAY_OFF);
    delay(20);
  }
  digitalWrite(RELAY_DIR_PIN, RELAY_DOWN);
  delay(20);
  digitalWrite(RELAY_POWER_PIN, RELAY_ON);

  directionUpDown = DIRECTION_DOWN;
  isMoving = true;
  coverState = DOWN;
  sendState();
}

void shuttersHalt(void) {
#ifdef MY_DEBUG
  Serial.println("Shutters halted");
#endif
  digitalWrite(RELAY_POWER_PIN, RELAY_OFF);
  delay(20);
  digitalWrite(RELAY_DIR_PIN, RELAY_UP);

  isMoving = false;
  requestedShutterLevel = currentShutterLevel;
#ifdef MY_DEBUG
  Serial.println("saving state to: ");
  Serial.println(String(currentShutterLevel));
#endif
  saveState(CHILD_ID_COVER, currentShutterLevel);
  coverState = STOP;
  sendState();
}

void changeShuttersLevel(int level) {
  int dir = (level > currentShutterLevel) ? DIRECTION_UP : DIRECTION_DOWN;
  if (isMoving && dir != directionUpDown) {
    shuttersHalt();
  }
  requestedShutterLevel = level;
}

void initShutters() {
#ifdef MY_DEBUG
  Serial.println("Init Cover");
#endif
  shuttersUp();
  delay((rollTime + timeOneLevel * LEVELS) * 1000);
  currentShutterLevel = STATE_UP;
  requestedShutterLevel = currentShutterLevel;
}

void receive(const MyMessage &message) {
#ifdef MY_DEBUG
  Serial.println("recieved incomming message");
  Serial.println("Recieved message for sensor: ");
  Serial.println(String(message.sensor));
  Serial.println("Recieved message with type: ");
  Serial.println(String(message.type));
#endif
  if (message.sensor == CHILD_ID_COVER) {
    switch (message.type) {
      case V_UP:
        //Serial.println(", New status: V_UP");
        changeShuttersLevel(STATE_UP);
        //state = UP;
        //sendState();
        break;

      case V_DOWN:
        //Serial.println(", New status: V_DOWN");
        changeShuttersLevel(STATE_DOWN);
        //state = DOWN;
        //sendState();
        break;

      case V_STOP:
        //Serial.println(", New status: V_STOP");
        shuttersHalt();
        //state = IDLE;
        //sendState();
        break;

      case V_PERCENTAGE:
        //Serial.println(", New status: V_PERCENTAGE");
        //          if (!initial_state_sent) {
        //            #ifdef MY_DEBUG
        //            Serial.println("Receiving initial value from controller");
        //            #endif
        //            initial_state_sent = true;
        //          }
        int per = message.getInt();
        if (per > STATE_UP) {
          per = STATE_UP;
        }
        changeShuttersLevel(per);
        //InitShutters(message.getInt());//send value < 0 or > 100 to calibrate
        //sendState();
        break;
    }
  } 
else if (message.sensor ==  CHILD_ID_SET) {

    if (message.type == V_VAR1) {
      Serial.println(", New status: V_VAR1, with payload: ");
      String strRollTime = message.getString();
      rollTime = strRollTime.toFloat();
      Serial.println("rolltime value: ");
      Serial.println(String(rollTime));
      saveState(CHILD_ID_SET, rollTime);
    }
  }
#ifdef MY_DEBUG
  Serial.println("exiting incoming message");
#endif
  return;
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

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCH_NAME, SKETCH_VER);
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_COVER, S_COVER, PRESENT_MESSAGE, IS_ACK);
  //present(CHILD_ID_SET, S_CUSTOM);
}

void setup(void) {
  //set up roll time if the saved value is not 255
  Serial.println("getting rolltime from eeprom: ");
  float tmpRollTime = loadState(CHILD_ID_SET);
  if (tmpRollTime != 0xff) {
    rollTime = tmpRollTime;
  }
  Serial.println(String(rollTime));

  int state = loadState(CHILD_ID_COVER);
#ifdef MY_DEBUG
  Serial.println("getting state from eeprom: ");
  Serial.println(String(state));
#endif
  if (state == 0xff) {
    initShutters();
  } else {
    changeShuttersLevel(state);
  }
}

void loop(void) {
  if (!initial_state_sent) {
#ifdef MY_DEBUG
    Serial.println("Sending initial value");
#endif
    sendState();
    
   // send(msgCode.set('20.0'));
    //    #ifdef MY_DEBUG
    //    Serial.println("Requesting initial value from controller");
    //    #endif
    //    request(CHILD_ID_COVER, V_PERCENTAGE);
    //    wait(2000, C_SET, V_PERCENTAGE);
    initial_state_sent = true;
  }

  debouncerUp.update();
  value = debouncerUp.read();
  if (value == 0 && value != oldValueUp) {
    changeShuttersLevel(STATE_UP);
    //state = UP;
    //sendState();
  }
  oldValueUp = value;

  debouncerDown.update();
  value = debouncerDown.read();
  if (value == 0 && value != oldValueDown) {
    changeShuttersLevel(STATE_DOWN);
    //state = DOWN;
    //sendState();
  }
  oldValueDown = value;

  debouncerStop.update();
  value = debouncerStop.read();
  if (value == 0 && value != oldValueStop) {
    shuttersHalt();
    //state = IDLE;
    //sendState();
  }
  oldValueStop = value;


  if (isMoving) {
    unsigned long _now = millis();
    if (_now - lastLevelTime >= timeOneLevel * 1000) {
      if (directionUpDown == DIRECTION_UP) {
        currentShutterLevel += 1;
      } else {
        currentShutterLevel -= 1;
      }
#ifdef MY_DEBUG
      Serial.println(String(requestedShutterLevel));
      Serial.println(String(currentShutterLevel));
#endif
      lastLevelTime = millis();
      //send(msgPercentage.set(currentShutterLevel));
    }
    if (currentShutterLevel == requestedShutterLevel) {
      shuttersHalt();
    }
  } else if (requestedShutterLevel != currentShutterLevel) {
    if (requestedShutterLevel > currentShutterLevel) {
      shuttersUp();
    }
    else {
      shuttersDown();
    }
    lastLevelTime = millis();
  }
}

