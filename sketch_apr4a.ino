//Lcd.print(F(“W”)); save sram space only use with mid size strings
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <EEPROM.h>
#ifndef IDLE
  #define IDLE 0
#endif 
   //macros
#ifndef ADD
  #define ADD 1
#endif

#ifndef CHANGE_STATE
  #define CHANGE_STATE 2
#endif

#ifndef SET_POWER
  #define SET_POWER 3
#endif

#ifndef REMOVE
  #define REMOVE 4
#endif

#ifndef SELECTMODE
  #define SELECTMODE 5
#endif


/***TEST VALUES: RIGHT         
  A-PZW-S-LivingRoom
  A-BDA-T-Entrance
  A-HJL-C-Garden
  A-FDG-O-Kitchen
  S-PZW-ON
  S-BDA-ON
  P-PZW-100
  P-BDA-20
**/

/**TEST VALUES: WRONG
ADFD-C-LivingRoom
A-DFD-Z-Kitchen
A-DFD—Kitchen
A-DFD-O-
A-FDG-C-
S-HJL-1
P-BDA-35




**/
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

byte uparrow[8] = {
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};
byte dnarrow[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};
byte msg_type = IDLE; 
String msg;
String errormsg;
String newidstring; String devtypestring; String locationstring;
unsigned long buttonPressedtime = 0;
bool buttonReleased = false;
byte btnprevstate = 0;
int state = 0;
const String allowedforID = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";   //an array listing the allowed characters
const String allowedforType = "SOLTC"; 
const String allowedforLoc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890"; 
//const String allowedforPowerPerc = "0123456789";   
//const String allowedforPowerTemp = "0123456789";  
int dash1 = 1;  //finds location of first -
int dash2 = 5;   //finds location of second -
int dash3 = 7;

String devicetopinfolist[10];   //15 is estimated max devices to be added, list of device info
String statestr;
String infoexclid; //capture all info but the device id
String topstr;
String botstr;
int deviceCount = 0;
int i = 0;

/*class Device {  
  public:             
    char deviceID[3];       
    char devlocation[16]; //1-15 chars + (0-9)
    char devtype; //S/O/L/T/C
    char devstate[3]; //on/off 
    int posAdded;
    explicit Device(char theid[4], char loc[17], char type) {    //save deviceids in arr
      posAdded = deviceCount;   //0 = added first, 1 = added second
      deviceID[0] = theid[0]; deviceID[1] = theid[1]; deviceID[2] = theid[2];  
      for (int x = 0; x < 16; x++) {devlocation[x] = loc[x];}
      devtype = type;
      devstate[0] = "O"; devstate[1] = "F"; devstate[2] = "F";
   //device info to be displayed as string   
      deviceCount++;
    }  
    int getAddedPos() {
      return posAdded;
    }
    void setState(String state) {
      if (state == "OFF") {
        devstate[0] = "O";
        devstate[1] = "F";
        devstate[2] = "F";
      } 
      else if (state == "ON") {    //' ''O''N'
        devstate[0] = " ";
        devstate[1] = "O";
        devstate[2] = "N";
      }
    }
};

class SpkrLight: public Device {
  using Device::Device;
  public:
    byte powerperc; //0-100% 
    setPower(byte pperc) {
      powerperc = pperc;
    }
};

class Thermo: public Device {
  using Device::Device;
  public:
    byte powertemp; //9-32'C
    setPower(byte ptemp) {
      powertemp = ptemp;
    }
};*/

void premain_mode() {   //setup
  lcd.setBacklight(5);
  bool count = true;
  while (count==true) {
    Serial.print("Q");
    delay(1000);
    if (Serial.available() > 0) {
      String input=Serial.readString();
      if (input == "X") {
        count = false;
      }
      else if (input=="\n" || input=="\r") {
        Serial.print(" ERROR: newline/carriage return entered. ");
      }
    }
  }
  Serial.print(F("BASIC\n"));
  lcd.setBacklight(7); 
}       
                                                                                                                                                                                                                                                     
  //lcd.setCursor(0,0);
  //lcd.write(byte(0)); 
  //lcd.setCursor(0,1);
  //lcd.write(byte(1)); 

void setup() {
  Serial.begin(9600);
  lcd.begin(16,2);
  premain_mode();
  lcd.createChar(0, uparrow);
  lcd.createChar(1, dnarrow);  
  lcd.setCursor(0,0);
}

void loop() {
  uint8_t pressedButtons = lcd.readButtons();
  int index = devicetopinfolist[i].indexOf(' ');   //first space in info string
  infoexclid = devicetopinfolist[i].substring(index+1);   //device info excluding its device id
  index = infoexclid.indexOf(' ')+devicetopinfolist[i].indexOf(' ')+1;   //second space in info string
  topstr = devicetopinfolist[i].substring(0, index);   //contains id and location
  botstr = devicetopinfolist[i].substring(index+1);   //contains type and state 
  sort(devicetopinfolist, deviceCount);
  statestr = devicetopinfolist[i].substring(devicetopinfolist[i].length()-3);   //contains device state
  if (statestr == "OFF") {
    lcd.setBacklight(3);
  }
  else if (statestr == " ON") {
    lcd.setBacklight(2);
  }
  if (deviceCount == 1) {   //no arrows 
    lcd.setCursor(1,0);   //print first device info
    lcd.print(topstr);
    lcd.setCursor(1,1);
    lcd.print(botstr);
  }
  else if (deviceCount > 1) {
    lcd.setCursor(0,1);
    lcd.write(byte(1));    //print down arrow
    lcd.setCursor(1,0);    //print first device info
    lcd.print(topstr);
    lcd.setCursor(1,1);
    lcd.print(botstr);
  }
  //menu items
  switch (msg_type) {
    case IDLE:   //show all devices, change backlight depending on state ON 2, OFF 3
    Serial.println("IDLE CASE IS ENTERED");
      sort(devicetopinfolist, deviceCount);
      if ((BUTTON_DOWN & pressedButtons) && (i < deviceCount)) {     //show next device only while end of list not reached
        i++;   //next device 
        lcd.clear();
        statestr = devicetopinfolist[i].substring(devicetopinfolist[i].length()-3);   //contains device state
        if (statestr == "OFF") {
          lcd.setBacklight(3);
        }
        else if (statestr == " ON") {
          lcd.setBacklight(2);
        }
        int index = devicetopinfolist[i].indexOf(' ');   //first space in info string
        infoexclid = devicetopinfolist[i].substring(index+1);   //device info excluding its device id
        index = infoexclid.indexOf(' ')+devicetopinfolist[i].indexOf(' ')+1;   //second space in info string
        topstr = devicetopinfolist[i].substring(0, index);   //contains id and location
        botstr = devicetopinfolist[i].substring(index+1);   //contains type and state 
        if (deviceCount == 2) {   //2 devices added in total
          lcd.setCursor(0,1);      //remove down arrow, add up arrow, last device 
          lcd.print(" ");
          lcd.setCursor(0,0);
          lcd.write(byte(0));
        } 
        else if (deviceCount > 2) {   //3 or more devices
          if (i >= 1 && i < deviceCount) {   //middle device 
            //show both arrows
            lcd.setCursor(0,0);
            lcd.write(byte(0));
            lcd.setCursor(0,1);
            lcd.write(byte(1));
          }
          else if (i == deviceCount-1) {    //last device
            //remove down arrow, add up arrow
            lcd.setCursor(0,1);
            lcd.print(" ");
            lcd.setCursor(0,0);
            lcd.write(byte(0));
          }          
        }
        lcd.setCursor(1,0);   //show device info on lcd
        lcd.print(topstr);
        lcd.setCursor(1,1);
        lcd.print(botstr);
      }
      else if ((BUTTON_UP & pressedButtons) && (i > 0)) {   //up button when first device is shown (i=0) does nothing: cond only works when down button has been pressed (i++)
        i--;
        lcd.clear();
        statestr = devicetopinfolist[i].substring(devicetopinfolist[i].length()-3);   //contains device state
        if (statestr == "OFF") {
          lcd.setBacklight(3);
        }
        else if (statestr == " ON") {
          lcd.setBacklight(2);
        }
        int index = devicetopinfolist[i].indexOf(' ');  //first space in info string
        infoexclid = devicetopinfolist[i].substring(index+1);   //device info excluding its device id
        index = infoexclid.indexOf(' ')+devicetopinfolist[i].indexOf(' ')+1;  //second space in info string  
        topstr = devicetopinfolist[i].substring(0, index);   //contains id and location
        botstr = devicetopinfolist[i].substring(index+1);   //contains type and state
        if (deviceCount == 2) {   //2 devices added in total   //i is 0, showing first device
          lcd.setCursor(0,0);      //remove up arrow, add down arrow
          lcd.print(" ");
          lcd.setCursor(0,1);
          lcd.write(byte(1));
        } 
        else if (deviceCount > 2) {   //3 or more devices
          if (i >= 1 && i < deviceCount) {   //middle device 
            //show both arrows
            lcd.setCursor(0,0);
            lcd.write(byte(0));
            lcd.setCursor(0,1);
            lcd.write(byte(1));
          }
          else if (i == deviceCount-1) {    //last device
            //remove down arrow, add up arrow
            lcd.setCursor(0,1);
            lcd.print(" ");
            lcd.setCursor(0,0);
            lcd.write(byte(0));
          }          
        }
        lcd.setCursor(1,0);   //show device info on lcd
        lcd.print(topstr);
        lcd.setCursor(1,1);
        lcd.print(botstr);
      } 
      int state = 0;
      if (state != btnprevstate) {
        if (BUTTON_SELECT & pressedButtons) {
          state = 1;
          buttonReleased = false;
          buttonPressedtime = millis();
        } else {
          buttonReleased = true;
        }
          btnprevstate = state;
      }
      if (buttonReleased == true) {
        long diff = millis()-buttonPressedtime;
        if (diff>1000) {
          msg_type=SELECTMODE;
        }
        buttonReleased = false;
        buttonPressedtime = 0;
      }
      //receive messages from serial
      if (Serial.available() > 0) {
        msg = Serial.readString();
        errormsg = "ERROR: "+msg;
        if (validatemsg(msg)==true) {Serial.println("OP2");}
        if (msg[0]=='A' && validatemsg(msg)==true) {
          msg_type=ADD;
          break;
        }
        else {Serial.println(errormsg);}
        if (deviceCount > 0) {   //no devices then ignore all messages
          if (validatemsg(msg)==true) {
            if (msg[0]=='S' && deviceCount > 0) {msg_type=CHANGE_STATE; break;}   //ignore other messages unless there are devices
            else if (msg[0]=='P' && deviceCount > 0) {msg_type=SET_POWER; break;}
            else if (msg[0]=='R' && deviceCount > 0) {msg_type=REMOVE; break;}
          }
          else {
            Serial.println(errormsg);
          }
        }
      } 

    case ADD:  //A-BCD-E-Location
      Serial.println("ADD CASE IS ENTERED");
      Serial.println(msg);
      if (validatemsg2(msg)==true && !deviceExists(getIDfromMsg(msg))) {  //message is valid and id is new
        String devtypestring = msg.substring(dash2+1,dash3+1);  //third data string: string from dash2+1 to dash3
        String locationstring = msg.substring(dash3+1); //fourth data string 
        char id_char_arr[4];
        char loc_arr[16];  
        char typchar_array[2];
        getIDfromMsg(msg).toCharArray(id_char_arr,4);
        locationstring.toCharArray(loc_arr,locationstring.length()+1);
        devtypestring.toCharArray(typchar_array,2);
        //IMPORTANT
        devicetopinfolist[deviceCount] = String(id_char_arr)+' '+String(loc_arr)+' '+String(typchar_array)+" OFF";   //add device info and defaults to OFF
        deviceCount++;   //new device added incr device counter
      } else if (validatemsg2(msg)==true && deviceExists(getIDfromMsg(msg))) {     //id already exists
          int deviceindex = findIDPos(getIDfromMsg(msg));   //where in the devicelist is that device id
          String devloc;
          String devicetype;
          String devicest = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-3);
          if (devicest == "OFF" || devicest == " ON") {    //a device without a power value
            String devloc = devicetopinfolist[deviceindex].substring(4, devicetopinfolist[deviceindex].length()-5);
            String devicetype = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-5, devicetopinfolist[deviceindex].length()-3);
            String devicest = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-3);   //gets device state ?
                       
          } else {      //a device with a power value at the end and not a state
            devloc = devicetopinfolist[deviceindex].substring(4, devicetopinfolist[deviceindex].length()-10);
            devicetype = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-10, devicetopinfolist[deviceindex].length()-8);   //gets device type
            devicest = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-8, devicetopinfolist[deviceindex].length()-4);
          }
          if (devicetype != msg.substring(dash2+1,dash3)) {   //if new devtype: change devtype
            devicetopinfolist[deviceindex].remove(5 + devloc.length(), 1);   
            insert(5+devloc.length(), msg.substring(dash3+1), devicetopinfolist[deviceindex]);
          }   
          if (devloc != msg.substring(dash3+1)) {   //if new location: change location
            devicetopinfolist[deviceindex].remove(4, devloc.length());    //ABC LOCATION L OFF  >  ABC LOCATI L OFF     ABC LOCATION L  ON
            //ABC LOCATION L OFF 100%      ABC LOCATION L  ON 100%
            insert(4, msg.substring(dash3+1), devicetopinfolist[deviceindex]);
          }      
      }
      //else {msg_type=IDLE;}
      break;

    case CHANGE_STATE:   //’S-BCD-ON’/’S-BCD-OFF’
      Serial.println("CHANGE STATE CASE IS ENTERED");
      Serial.println(msg);
      if (deviceExists(getIDfromMsg(msg))) {
        String dvstate = msg.substring(dash2+1); 
        if (dvstate == "ON") {
          dvstate = " ON";          
        }
        int deviceindex = findIDPos(getIDfromMsg(msg));   //where in the devicelist is that device id
        String devicest = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-3);   //device state is supposedly the last 3 chars unless power value
        if (devicest == "OFF" || devicest == " ON") {    //a device without a power value
          //remove 3 chars off the end
          devicetopinfolist[deviceindex].remove(devicetopinfolist[deviceindex].length()-3, 3);
          //add new state to the end
          devicetopinfolist[deviceindex] = devicetopinfolist[deviceindex] + dvstate;
        } else {      //a device with a power value at the end
          devicest = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-8, devicetopinfolist[deviceindex].length()-4);
          //remove 3 chars from position of state in the device info string
          devicetopinfolist[deviceindex].remove(devicetopinfolist[deviceindex].length()-8, 3);   //removes state
          //insert dvstate into the removed part
          insert(devicetopinfolist[deviceindex].length()-5, dvstate, devicetopinfolist[deviceindex]);
        }
      }
    //idle: deviceExists = 0
    //error: missing ID or value
    break;

    case SET_POWER:  //‘P-BCD-123’ where 123 is: Speaker AND lIGHT: volume 0-100, For Thermostat: temperature 9-32
      Serial.println("SET POWER CASE IS ENTERED");
      Serial.println(msg);
      String devicetype;
      bool pval;
      if (deviceExists(getIDfromMsg(msg))) {
        int deviceindex = findIDPos(getIDfromMsg(msg));   //where in the devicelist is that device id
        String devicest = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-3);   //gets device state if there's no power value
        if (devicest == "OFF" || devicest == " ON") {    //a device without a power value
          devicetype = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-5, devicetopinfolist[deviceindex].length()-3);   //gets device type
          pval = false;
        } else {      //a device with a power value 
          devicetype = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-10, devicetopinfolist[deviceindex].length()-8);   //gets device type
          //devicest = devicetopinfolist[deviceindex].substring(devicetopinfolist[deviceindex].length()-8, devicetopinfolist[deviceindex].length()-4);    //device state
          pval = true;
        }
        
        String powerval = msg.substring(dash2+1);    //power value entered by user
        if (validatemsg2(msg)==true && (devicetype != "O" || devicetype != "C")) {   //idle: deviceType = O/C, error: missing ID or value
          if ((devicetype == "S" || devicetype == "L") && (powerval.toInt()>=0 && powerval.toInt()<=100)) {
            if (powerval.length() == 1) {    //power value is 1 digit percent
              if (pval == true) {     //a device with an existing power value: change power value
                devicetopinfolist[deviceindex].remove(devicetopinfolist[deviceindex].length()-5, 5);   //remove last 5 chars before adding new power value
              }
              devicetopinfolist[deviceindex] = devicetopinfolist[deviceindex] + "   " + powerval + '%';  //add leading spaces
            }
            else if (powerval.length() == 2) {    //power value is 2 digits percent
              if (pval == true) {    
                devicetopinfolist[deviceindex].remove(devicetopinfolist[deviceindex].length()-5, 5);   
              }
              devicetopinfolist[deviceindex] = devicetopinfolist[deviceindex] + "  " + powerval + '%';
            } else {    //power value is 3 digits percent
              if (pval == true) {    
                devicetopinfolist[deviceindex].remove(devicetopinfolist[deviceindex].length()-5, 5);  
              }
              devicetopinfolist[deviceindex] = devicetopinfolist[deviceindex] + ' ' + powerval + '%';
            } 
          }
          else if (devicetype == "T" && (powerval.toInt()>=9 && powerval.toInt()<=32)) {
            if (powerval.length() == 1) {   //single digit degrees C
              if (pval == true) {    
                devicetopinfolist[deviceindex].remove(devicetopinfolist[deviceindex].length()-5, 5);  
              }
              devicetopinfolist[deviceindex] = devicetopinfolist[deviceindex] + "  " + powerval + "°C";
            } else {                //2 digit degrees C
              if (pval == true) {     
                devicetopinfolist[deviceindex].remove(devicetopinfolist[deviceindex].length()-5, 5);   
              }
              devicetopinfolist[deviceindex] = devicetopinfolist[deviceindex] + ' ' + powerval + "°C";
            }
          } else {   //error: value outside range
            Serial.println(errormsg);
          }
        }
      }
      break;

    case REMOVE:   //‘R-BCD’
      Serial.println("REMOVE CASE IS ENTERED");
      Serial.println(msg);
      if (validatemsg2(msg)==true && deviceExists(getIDfromMsg(msg))) {
        int deviceindex = findIDPos(getIDfromMsg(msg));   //where in the devicelist is that device id
        devicetopinfolist[deviceindex].remove(0);  //remove device info
        deviceCount--;
      }
    //idle: deviceExists = 0
    //error: missing ID
    break;

    case SELECTMODE:
      Serial.println("SELECTMODE CASE IS ENTERED");
      lcd.clear();
      lcd.setBacklight(5);
      lcd.setCursor(4,0);
      lcd.print(F("F231337"));
      //button not pressed then break
      break; 

    default:
      Serial.println("Default entered");
  }
  delay(300);
}

bool validatemsg(String message) {    //check message is in correct format   A-PZW-S-LivingRoom
  Serial.println("Executing first validation stage...");
  //if msg doesnt start with a,p,s,r, missing any -
  if (message.indexOf('-') != 1) {    //check if msg contains first '-' at correct position
    Serial.print("IF 1 EXEC");
    return false;
  }
  //CHECK IF SECOND DASH IS IN CORRECT PLACE
  else if ((message.indexOf('P') == 0 || message.indexOf('S') == 0 || message.indexOf('A') == 0) && ((message.substring(2)).indexOf('-') != 3)) {
    Serial.print("IF 2 EXEC");
    return false;
  }
  else if (message.indexOf('A') == 0 && (message.substring(6)).indexOf('-') != 1) {     //CHECK IF third DASH IS IN CORRECT PLACE
    Serial.print("IF 3 EXEC");
    return false;
  }
  else {
    return true;
  }
}

bool validatemsg2(String message) {  //if ‘A’ is used with existing device ID: use given new device type AND/OR new location //default state of new device is OFF
  Serial.println("Executing second validation stage...");
  newidstring = message.substring(dash1+1,dash2+1);   //captures second data String
  char id_char_arr[4];
  char devtype_arr;
  char loc_arr[16];  
  newidstring.toCharArray(id_char_arr,4);
  if (msg_type==ADD) {
    devtypestring = message.substring(dash2+1,dash3+1);  //third data string: string from dash2+1 to dash3
    locationstring = message.substring(dash3+1); //fourth data string 
    if (locationstring.length() > 15) {
    locationstring.toCharArray(loc_arr,16);  //cut off at 15 chars
    } else {
      locationstring.toCharArray(loc_arr,locationstring.length()+1);  //string is 15 chars or less
    }
    // missing ID/Device Type/Location
    if (!(checkvalidstring_ID(allowedforID,id_char_arr) && checkvalidstring_Type(allowedforType,devtype_arr) && checkvalidstring_Loc(allowedforLoc,loc_arr))) {
      Serial.println(errormsg);
      return false;
    } else {
      return true;
    }
  }
  
  else if (msg_type==CHANGE_STATE) {
    String devstatestr = message.substring(dash2+1);  //third data string
    if (!(checkvalidstring_ID(allowedforID,id_char_arr) && (devstatestr == "ON" || devstatestr == "OFF"))) {
      Serial.println(errormsg);
      return false;
    } else {
      return true;
    }
  }
//idle: if no devices yet    
 //idle: deviceExists = 0
    //error: missing ID or value
    //eror: msg doesnt contain '-'
   
  else if (msg_type==SET_POWER) {
    if (!(checkvalidstring_ID(allowedforID,id_char_arr))) {
      Serial.println(errormsg);
      return false;
    } else {
      return true;
    }
    //idle: if no devices yet
//idle: deviceType = O/C
    //error: value outside range
    //error: missing ID or value
    //eror: msg doesnt contain '-'
  }
    
      
  else if (msg_type==REMOVE) {
    if (!(checkvalidstring_ID(allowedforID,id_char_arr))) {
      Serial.println(errormsg);
      return false;
    } else {
      return true;
    }
  }
}

//'delete device' deletes an instance
//if(content.indexOf("Teststring") > 0), returns -1 if not in
//checkvalidstring(allowedforID,id_char_arr)   id_char_arr is char[4]

bool checkvalidstring_ID (const String allowedchars, char msgpart[4]) {
  Serial.println("Executing check valid ID format...");
  byte validString = 0; // counter of allowed characters
  for (byte i = 0; i < strlen(msgpart); i++) {  
    if (allowedchars.indexOf(msgpart) != -1) { //search for msgpart[i] in allowed characters 
    //set index to -1 at start and anything other at end of for - match found
      validString++;
    //for loop and look for match, determine if match or no match at the end, flag is true each match, last match save in variable - save index
    }  //char doesnt exist in allowed_arr
  }
  if (validString == strlen(msgpart)) {  //all characters are valid 
    return true;
  } else { //one of the characters is not allowed e.g. a space
    return false;
  }
}

bool checkvalidstring_Type (const String allowedchars, char msgpart) {
  Serial.println("Executing check valid device type format...");
  byte validString = 0; // counter of allowed characters
  if (allowedchars.indexOf(msgpart) != -1) { //search for msgpart[i] in allowed_arr .... msgpart[i] is of type char
    validString=1;  
  } 
  if (validString == 1) {  //all characters are valid
    return true;
  } else { //one of the characters is not allowed e.g. a space
    return false;
  }
}

bool checkvalidstring_Loc (const String allowedchars, char msgpart[16]) {
  Serial.println("Executing check valid location format...");
  byte validString = 0; // counter of allowed characters
  for (byte i = 0; i < strlen(msgpart); i++) {  
    if (allowedchars.indexOf(msgpart[i]) != -1) { //search for msgpart[i] in allowed_arr .... msgpart[i] is of type char
      validString++;  
    } 
  }
  if (validString == strlen(msgpart)) {  //all characters are valid
    return true;
  } else { //one of the characters is not allowed e.g. a space
    return false;
  }
}

bool deviceExists(String dvcid) {   //deviceid matches an existing objects deviceid
  for (int i = 0; i < deviceCount; i++) {
    String devicelistsplit = devicetopinfolist[i].substring(0,3);  //gets device id (first 3 chars) from list of device information
    if (devicelistsplit == dvcid) {   //check if deviceid exists in device info list
      return true;
    }
  }
  return false;
}
void sort(String a[], int size) {
  for (int i = 0; i < (size - 1); i++) {   //i < 2
    for (int o = 0; o < (size - (i + 1)); o++) {  //3-1= o < 2
      if (a[o] > a[o + 1]) {
        String t = a[o];   // swap
        a[o] = a[o + 1];   // " "
        a[o + 1] = t;   // " "
      }
    }
  }
}

//captures id entered from protocol
String getIDfromMsg(String message) {       //get device id info from the message sent to serial
  String str = message.substring(dash1+1,dash2+1);   
  return str;
}

int findIDPos(String id) {    //only called if deviceExists = true, return position in device list
  for (int i = 0; i < deviceCount; i++) {
    if (devicetopinfolist[i].substring(0,3) == id) {   //check where deviceid is in device info list + return position
      return i;
    }
  }
}

String insert(int startpos, String insertString, String str) {    //for changing device information after the old info is removed
  String endstr = str.substring(startpos);
  str.remove(startpos);
  str = str + insertString;
  str = str + endstr;
  return str;
}
