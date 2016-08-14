#include <Tadiran.h>
#include <IRremote.h>



// the IR emitter object
IRsend irsend;
int incomingByte = 0;
// A/C Settings
int temperature = 26;
int mode = MODE_auto; // 0-Auto | 1-Cold | 2-Dry | 3-Fan | 4-Heat
int fanspeed = FAN_auto; //0-Auto | 1-Low | 2-Medium | 3-High
//A/C Toggles
boolean power = false;

// the Tairan code generator object
Tadiran tadiran(mode, fanspeed, temperature, STATE_off);

void setup()
{
  Serial.begin(115200);
  Serial.println("Commands: +/- Temperature | m - Mode | f - fanspeed | p - Power");
}

void loop() {
    
 if (Serial.available() > 0) {
        //Serial.write(27);  // ESC command
        //Serial.print("[2J");
        // read the incoming byte:
        incomingByte = Serial.read();
       
        // say what you got:
        Serial.print("Command: ");
        if(incomingByte == 43){
            Serial.print("Temperature +");
            if(temperature +1 <=30){
                temperature++;
            }
            tadiran.setTemeprature(temperature);
        }
        if(incomingByte == 45){
            Serial.print("Temperature -");
            if(temperature -1 >=16){
                temperature--;
            }
            tadiran.setTemeprature(temperature);
        }
        if(incomingByte == 109){ //mode
            Serial.print("Mode");
            if(mode+1 <=4) {
                mode++;
            } else {
                mode=0;
            }
            tadiran.setMode(mode);
        }
        if(incomingByte == 102) { //fan
            Serial.print("Fan");
            if(fanspeed+1 <=3) {
                fanspeed++;
            } else {
                fanspeed=0;
            }
            tadiran.setFan(fanspeed);
        }
        if(incomingByte == 112){ //power
            Serial.print("Power");
            tadiran.setState(STATE_on);
        }
        Serial.println("");
        
		tadiran.print();
        
        Serial.println("");
        Serial.println("Commands: +/- Temperature | m - Mode | f - fanspeed | p - Power");
        irsend.sendRaw(tadiran.codes, TADIRAN_BUFFER_SIZE, 38);
    }
}
  