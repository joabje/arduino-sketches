/*
    Wait for ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.
   
    If the card has a 4-byte UID it is probably a Mifare
    Classic card, and the following steps are taken:
   
    - Authenticate block 4 (the first block of Sector 1) using
      the default KEYA of 0XFF 0XFF 0XFF 0XFF 0XFF 0XFF
    - If authentication succeeds, we can then read any of the
      4 blocks in that sector (though only block 4 is read here)
	 
    If the card has a 7-byte UID it is probably a Mifare
    Ultralight card, and the 4 byte pages can be read directly.
    Page 4 is read by default since this is the first 'general-
    purpose' page on the tags.
    
    Send it to the web server

*/
//#define DEBUG1
#include <SPI.h> // NFC
#include <Wire.h> // NFC
#include <Adafruit_PN532.h> // NFC
#include <SoftwareSerial.h> // ESP

// NFC Wiring:
//Use 5V
#define SCK  (2)
#define MOSI (3)
#define SS   (4)
#define MISO (5)
Adafruit_PN532 nfc(SCK, MISO, MOSI, SS);
String uidString;


// ESP:
// Use 5V
//#define _SS_MAX_RX_BUFF 256 // RX buffer size //BEFORE WAS 64   // ESP
SoftwareSerial esp8266Module(10, 9); // RX, TX   // ESP
/*
// ESP Wiring:
    10  GND ----,
    3V  NULL | |,
    3V  NULL |--,
    5V  9   | |     // --> I thought 3V but 5 V seems to work better!
               | 
               |
*/
//const String wifiNetwork = "";
//const String wifiPassword = "";
const String wifiNetwork = "";
const String wifiPassword = "";
const String herokuAuthkey = "";
const String weatherCode = "3143244"; //BBC Weather code Oslo - 3143244
String tempWifi;
int wifiStatus = 1;
int wifiLight;
// Temp variables
String wifiVal1 = "----------";
String wifiVal2 = "----------";
String wifiVal3 = "----------";
String wifiVal4 = "----------";
String wifiVal5 = "----------";
int wifiLoopNum = 0;

/*
#ifdef DEBUG1
  TempsAndTime tempsAndTimes[40];
#else  
  TempsAndTime tempsAndTimes[50];
#endif
*/
void setup(void) {
  pinMode(8, OUTPUT); //ESP8266 RESET PIN
  digitalWrite(8, HIGH);  //ESP8266 RESET PIN
  
  Serial.begin(9600);
 
  Serial.println("Setting up NFC");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print(F("Found chip PN5")); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print(F("Firmware ver. ")); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  
  Serial.println("Setting up ESP...");
  esp8266Module.begin(9600);
  delay(5000);

  
  Serial.println(F("Waiting for an ISO14443A Card ..."));
}

void loop(void) {
  // NFC...
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
   
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  Serial.println(F("Waiting for something to read..."));
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    Serial.println(F("Found an ISO14443A card. UID Length: "));
    Serial.print(uidLength, DEC);
    Serial.println(" bytes. UID Value: ");
  //  nfc.PrintHex(uid, uidLength);
    Serial.println(getHexArrayAsString(uid, uidLength));
    Serial.println(F(""));
    
    if (uidLength == 4) {
      // We probably have a Mifare Classic card ... 
      Serial.println(F("Seems to be a Mifare Classic card (4 byte UID)"));
	  
      // Try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println(F("Trying to authenticate block 4 with default KEYA value"));
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	  
	  // Start with block 4 (the first block of sector 1) since sector 0
	  // contains the manufacturer data and it's probably better just
	  // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);
	  
      if (success) {
        Serial.println(F("(Sector 1 (Blocks 4..7) has been authenticated"));
        uint8_t data[16];
		
        // If you want to write something to block 4 to test with, uncomment
		// the following line and this text should be read back in a minute
        // data = { 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0};
        // success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(4, data);
		
        if (success) {
          // Data seems to have been read ... spit it out
          Serial.println(F("Reading Block 4:"));
          nfc.PrintHexChar(data, 16);
          Serial.println(getHexArrayAsString(data, 16));
          Serial.println(F(""));
		  
          // Wait a bit before reading the card again
          delay(1000);
        } else {
          Serial.println(F("Ooops ... unable to read the requested block.  Try another key?"));
        }
      } else {
        Serial.println(F("Ooops ... authentication failed: Try another key?"));
      }
    }
    if (uidLength == 7) {
      // We probably have a Mifare Ultralight card ...
      Serial.println(F("Seems to be a Mifare Ultralight tag (7 byte UID)"));
      // Try to read the first general-purpose user page (#4)
      Serial.println(F("Reading page 4"));
      uint8_t data[32];
      success = nfc.mifareultralight_ReadPage (4, data);
      if (success) {
        // Data seems to have been read ... spit it out
      //  nfc.PrintHexChar(data, 4);
        Serial.println(getHexArrayAsString(data, 4));
        Serial.println(F(""));
        // Wait a bit before reading the card again
        delay(1000);
      }
      else {
        Serial.println(F("Ooops ... unable to read the requested page!?"));
      }
    }
  } else {
      Serial.println(String("Success 2 was: " + success));
  }
  
  // NFC end...
  
  
  delay(5000);
  runEsp8266(F("ciberlunsj.herokuapp.com"), F("/adddevice"));
}

String getHexArrayAsString(const byte * data, const uint32_t numBytes) {
  uidString = ""; 
  uint32_t szPos;
  for (szPos=0; szPos < numBytes; szPos++) {
    // Append leading 0 for small values
    if (data[szPos] <= 0xF) {
      uidString += F("0");
    }
    uidString += data[szPos];
    /*if ((numBytes > 1) && (szPos != numBytes - 1)) {
      Serial.print(" ");
    }*/
  }
  return uidString;
}

void runEsp8266(String website, String page) {
  // 0 need to reset or beginning of loop
  // 1 reset complete check wifi mode
  // 2 wifi mode is 3, now check network connection
  // 3 If not connected connect to network
  // 4 request page from server
  // 5 unlink from server after request
  // 6 close network connection
  switch (wifiStatus) {
    case 0:    // 0 need to reset or beginning of loop
      Serial.println(F("TRYING esp8266Reset"));
      esp8266Reset();
      break;
    case 1:    // 1 reset complete check wifi mode
      Serial.println(F("TRYING changeWifiMode"));
      changeWifiMode();
      break;
    case 2:    // 2 wifi mode is 3, now check network connection
      Serial.println(F("TRYING checkWifiStatus"));
      checkWifiStatus();
      break;
    case 3:    // 3 If not connected connect to network
      Serial.println(F("TRYING connectToWifi"));
      connectToWifi();
      //connectToWifi("networkIdetifier", "networkPassword");
      break;
    case 4:    // 4 request page from server
      Serial.println(F("TRYING getPage"));
      getPage(website, page);
      //getPage(website, page, "?num=", "3", "&num2=", "2000");
      break;
    case 5:    // 5 unlink from server after request
      Serial.println(F("TRYING unlinkPage"));
      unlinkPage();
      break;
  }
}

// 0 - RESET
bool esp8266Reset() {
  esp8266Module.println(F("AT+RST"));
  delay(7000);
  if (esp8266Module.find("OK")) {
    wifiVal1 = F("-RESET-");
    wifiStatus = 1;
    return true;
  } else  {
    wifiVal1 = F("-FAILED-");
    wifiStatus = 0;
    return false;
  }
}

// 1 - CHANGE MODE
bool changeWifiMode() {
  Serial.println("1");
  esp8266Module.println(F("AT+CWMODE?"));
  delay(5000);
  if(esp8266Module.find("3"))   {
    wifiVal1 = F("Wifi Mode is 3");
    wifiStatus = 2;
    return true;
  } else {
    esp8266Module.println(F("AT+CWMODE=3"));
    delay(5000);
    if (esp8266Module.find("no change") || esp8266Module.find("OK")) {
      wifiVal1 = F("Wifi Mode is 3");
      wifiStatus = 2;
      return true;
    } else {
      wifiVal1 = F("Wifi Mode failed");
      wifiStatus = 0;
      return false;
    }
  }
}

// 2 - CHECK WIFI NETWORK STATUS
bool checkWifiStatus() {
  esp8266Module.println("AT+CWJAP?");

  delay(5000);
  if (esp8266Module.find(":")) {
    Serial.println(F("WIFI NETWORK CONNECTED"));
    wifiVal1 = F("WIFI:");
    wifiVal1 += esp8266Module.readStringUntil('\n');
    wifiStatus = 4;
    return true;
  } else {
    wifiStatus = 3;
    return false;
  }
}

// 3 - CONNECT TO WIFI
bool connectToWifi() {
  String cmd = F("AT+CWJAP=\"");
  cmd += wifiNetwork;
  cmd += F("\",\"");
  cmd += wifiPassword;
  cmd += F("\"");
  esp8266Module.println(cmd);
  delay(5000);
  if (esp8266Module.find("OK")) {
    wifiVal1 = F("CONNECTED TO WIFI");
    wifiStatus = 4;
    return true;
  } else {
    wifiStatus = 0;
    return false;
  }
}

// 4 - GET PAGE
bool getPage(String website, String page) {
  String cmd = F("AT+CIPSTART=\"TCP\",\"");
  String empNo = F("249");
  String deviceId = F("F1337fcId");
  cmd += website;
  cmd += F("\",80");
  esp8266Module.println(cmd);
  delay(5000);
  if (esp8266Module.find("Linked")) {
    Serial.print(F("Connected to server"));
  }
  cmd =  F("GET ");
  cmd += page;
  cmd += F("?authkey=");  //construct the http GET request
  cmd += herokuAuthkey;
  cmd += F("&empno=");
  cmd += empNo;
  cmd += F("&deviceid=");
  cmd += deviceId;
  cmd += F(" HTTP/1.0\r\n");
  cmd += F("Host: ");
  cmd += website;
  cmd += F("\r\n\r\n");
  Serial.println(cmd);
  esp8266Module.print(F("AT+CIPSEND="));
  esp8266Module.println(cmd.length());
  Serial.println(cmd.length());
  Serial.println("");
  if (esp8266Module.find(">")) {
    Serial.println(F("found > prompt - issuing GET request"));
    esp8266Module.println(cmd);
  } else {
    wifiStatus = 5;
    Serial.println(F("No '>' prompt received after AT+CPISEND"));
    wifiVal1 = F("Failed request, retrying...");
    return false;
  }

  while (esp8266Module.available() > 0) {
    esp8266Module.read();
  }

  if (esp8266Module.find("*")) {
    String tempMsg = esp8266Module.readStringUntil('\n');
    wifiVal2 = splitToValWifi(tempMsg, F("@"), F("|"));
    wifiVal3 = splitToValWifi(tempMsg, F("+"), F("@"));
    String piecetemp = splitToValWifi(tempMsg, F("|"), F("$"));
    wifiVal5 = splitToValWifi(tempMsg, F("$"), F("^"));
    wifiVal4 = splitToValWifi(tempMsg, F("^"), F("~"));
    int peice = piecetemp.toInt();
    Serial.println(wifiVal2);
    Serial.println(wifiVal3);
    Serial.println(wifiVal4);
    Serial.println(wifiVal5);
    Serial.println(piecetemp);

    wifiStatus = 5;
    return true;
  } else {
    wifiStatus = 5;
    return false;
  }
}

// 5 - UNLINK
bool unlinkPage() {
  esp8266Module.println(F("AT+CIPCLOSE"));
  delay(5000);
  if (esp8266Module.find("Unlink")) {
    wifiVal1 = F("UNLINKED");
    wifiStatus = 0;
    wifiLoopNum++;
    return true;
  }
  else {
    wifiStatus = 4;
    return false;
  }
}

// SPLIT UP STRINGS - ESP method
String splitToValWifi(String inputString, String delimiter, String endChar) {
  String tempString = F("");
  int from;
  int to;
  for (int i = 0; i < inputString.length(); i++) {
    if (inputString.substring(i, i + 1) == delimiter) {
      from = i + 1;
    }
    if (inputString.substring(i, i + 1) == endChar) {
      to = i;
    }
  }
  tempString = inputString.substring(from, to);
  return tempString;
}
