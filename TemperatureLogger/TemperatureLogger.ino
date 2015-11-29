//#define DEBUG1
#include <Time.h>
#include <SPI.h>
#include <Ethernet.h>
#include <DallasTemperature.h>
#include <OneWire.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Enter a MAC address for controller. Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte server[]  = { 184, 106, 153, 149 }; // ThingSpeak IP Address: 184.106.153.149
OneWire oneWire(2); // Data wire on port 2 on Uno
DallasTemperature sensors(&oneWire); // Pass oneWire reference to Dallas Temperature.
DeviceAddress thermo1, thermo2, thermo3; // arrays to hold device addresses

const String apiKey = "<INSERT_YOUR_KEY>"; // Thingspeak API key
IPAddress ip(192, 168, 0, 177); // Static IP address to use if the DHCP fails to assign
EthernetClient client; // Initialize the Ethernet client library with the IP address and port of the server that you want to connect to (port 80 is default for HTTP)
const int initialRequestInterval = 30000; // Delay between requests
int requestInterval = initialRequestInterval;
long lastAttemptTime = 0; // last time you connected to the server, in milliseconds
bool timeIsSet = false;
String dateStr;
// TODO JB: Generalize to other than 3 temperatures 
typedef struct {
  float temp1;
  float temp2;
  float temp3;
  long timestamp;
} TempsAndTime;
#ifdef DEBUG1
  TempsAndTime tempsAndTimes[40];
#else  
  TempsAndTime tempsAndTimes[50];
#endif
int tatReadIndex = 0;
int tatNextWriteIndex = 0;
TempsAndTime tempTat;
short i;

void setup() {
  Serial.begin(9600); 
  sensors.begin(); // Start up the DallasTemperature library

  sensors.setResolution(thermo1, 12);
  sensors.setResolution(thermo2, 12);
  sensors.setResolution(thermo3, 12);

  //TODO JB: Change this: Do with search for instead instance + extract method.
   #ifdef DEBUG1
    Serial.println(String(sensors.getDeviceCount(), DEC) + " temperature devices.");
    if(!sensors.getAddress(thermo1, 0)) {
      Serial.println(F("Unable to find address for Device 0"));
    }
    if(!sensors.getAddress(thermo2, 1)) {
      Serial.println(F("Unable to find address for Device 1"));
    }
    if(!sensors.getAddress(thermo3, 2)) {
      Serial.println(F("Unable to find address for Device 2"));
    }
  #else
     sensors.getAddress(thermo1, 0);
     sensors.getAddress(thermo2, 1);
     sensors.getAddress(thermo3, 2);
  #endif
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  if(Ethernet.begin(mac) == 0) { // start the Ethernet connection:
    #ifdef DEBUG1
      Serial.println(F("Failed to configure Ethernet using DHCP"));
    #endif
    Ethernet.begin(mac, ip); // try to configure using IP address instead of DHCP
  }
  delay(2000); // give the Ethernet shield time to initialize

  if (client.connect(server, 80)) {  // if you get a connection, report back via serial
    #ifdef DEBUG1
      Serial.println(F("Connected."));
    #endif
  }
  #ifdef DEBUG1
    else {
      Serial.println(F("Connection failed."));    // didn't get a connection to the server
    }
  #endif
}

void loop() {
  delay(1000);
  sensors.requestTemperatures();
  delay(1000);
  if (client.connected()) {
    delay(1000);
    #ifdef DEBUG1
      Serial.println(F("Is connected!"));
    #endif
    if(client.available()) {
      delay(1000);
      if(!timeIsSet) {setTimeBasedOnAServer();}
    }
    delay(1000);
    client.stop();
    delay(1000);
  } else if (millis() - lastAttemptTime > requestInterval) {
    #ifdef DEBUG1
      Serial.println(String("Temps ") + sensors.getTempC(thermo1) + ", " + sensors.getTempC(thermo2) + ", " + sensors.getTempC(thermo3));
    #endif
    connectToServer(sensors.getTempC(thermo1), sensors.getTempC(thermo2), sensors.getTempC(thermo3));

    #ifdef DEBUG1
      Serial.println(String("Readable time: ") + getReadableDate(now()));
      Serial.println(String(timeIsSet));
    #endif
  }
}

// TODO JB: Re-write to be dynamic with the number of temperatures used. Loop through an array sent in as a parameter sensors.getDeviceCount()
void connectToServer(float temp1, float temp2, float temp3) {
  delay(2000);
  client.stop();
  delay(2000);
  if(client.connect(server, 80)) {
    client.println(String("GET /update?1=") + temp1 + "&2=" + temp2 + "&3=" + temp3 + " HTTP/1.1");
    client.println(F("getString"));
    client.println(F("Host: api.thingspeak.com"));
    client.println(F("Connection: close"));
    client.println("X-THINGSPEAKAPIKEY: " + apiKey);
    client.println(F("Content-Type: application/x-www-form-urlencoded"));
    client.println(F(""));

    delay(2000);
    
    if(tatReadIndex < tatNextWriteIndex) {
      callThingspeakFromBackup();
    }
    requestInterval = initialRequestInterval;
  } else {
      #ifdef DEBUG1
        Serial.println(F("Connect was unsuccessful.. Trying to store it for later sending"));
      #endif
      insertIntoTempsAndTimes(temp1, temp2, temp3, now());
      // requestInterval = 1800000; // 30 min  // TODO JB: Consider to change the logging interval when network is down (and set it back to initial when network is up again)
  }
  lastAttemptTime = millis();
}

void callThingspeakFromBackup() {
  #ifdef DEBUG1
    Serial.println(String("callThingspeakFromBackup(), readIndex:") + tatReadIndex + ",writeIndex:" + tatNextWriteIndex);
  #endif
  if(timeIsSet) {
    for(i = 0; (i < 10 && tatReadIndex < tatNextWriteIndex); i++) {
      tempTat = tempsAndTimes[tatReadIndex];

      delay(5000);
      client.stop();
      delay(3000);
      
      if(client.connect(server, 80)) {
        delay(3000);
        dateStr = getReadableDate(tempTat.timestamp);
        #ifdef DEBUG1
          Serial.println(String(i));
          Serial.println(String("tempTat:") + tempTat.temp1 + "," + tempTat.temp2 + "," + tempTat.temp3 + "," + dateStr);
        #endif
        client.println(String("GET /update?1=") + tempTat.temp1 + "&2=" + tempTat.temp2 + "&3=" + tempTat.temp3 + "&created_at=" + dateStr + "Z HTTP/1.1");
        client.println(F("Host: api.thingspeak.com"));
        client.println(F("Connection: close"));
        client.println("X-THINGSPEAKAPIKEY: " + apiKey);
        client.println(F("Content-Type: application/x-www-form-urlencoded"));
        client.println(F(""));
        delay(5000);
        tatReadIndex++;
      } 
      #ifdef DEBUG1
      else {      
        Serial.println(F("Backupconnect was unsuccessful for i:"));
        Serial.println(String(i));
      }
      #endif
    }
  }
  #ifdef DEBUG1
    else {
      Serial.println(F("timeIsSet was false."));
    }
  #endif
}

void insertIntoTempsAndTimes(float temp1, float temp2, float temp3, long timestamp) {
  #ifdef DEBUG1
    Serial.println(String("insertIntoTempsAndTimes,") + temp1 + "," + temp2 + "," + temp3 + "," + timestamp);
  #endif
  if(timeIsSet) {
    tempTat = {temp1, temp2, temp3, timestamp};
    if(tatNextWriteIndex >= sizeof(tempsAndTimes)) {
      // TODO JB: Now we are writing over data we might not have used...
      #ifdef DEBUG1
        Serial.println(F("tatNextWriteIndex was >= sizeof array. Resetting tatNextWriteIndex..."));
      #endif
      tatNextWriteIndex = 0;
    }
    tempsAndTimes[tatNextWriteIndex] = tempTat;
    tatNextWriteIndex++;
  }
  #ifdef DEBUG1
    else {Serial.println(F("timeIsSet was false"));}
  #endif
}

/*
 * Convert now()-timestamp to a readable Date that Thingspeak can use
 */
String getReadableDate(long timestamp) {
  dateStr = "";
  if(timeIsSet) {
    dateStr = String(year(timestamp)) + "-" + month(timestamp) + "-" + day(timestamp) + "T" + hour(timestamp) + ":" +  minute(timestamp) + ":" + second(timestamp);
  }
  return dateStr;
}

/*
 * Sets the time now in unix time stamp
 */
void setTimeBasedOnAServer() {
  unsigned long time = 0;
  if (client.available()) {
    //char c = client.read();
    char buf[5];
    if (client.find((char *)"\r\nDate: ") // look for Date: header
        && client.readBytes(buf, 5) == 5) // discard
    {
      unsigned day = client.parseInt();	   // day
      client.readBytes(buf, 1);	   // discard
      client.readBytes(buf, 3);	   // month
      int year = client.parseInt();	   // year
      byte hour = client.parseInt();   // hour
      byte minute = client.parseInt(); // minute
      byte second = client.parseInt(); // second

      int daysInPrevMonths;
      switch (buf[0]) {
        case 'F': daysInPrevMonths =  31; break; // Feb
        case 'S': daysInPrevMonths = 243; break; // Sep
        case 'O': daysInPrevMonths = 273; break; // Oct
        case 'N': daysInPrevMonths = 304; break; // Nov
        case 'D': daysInPrevMonths = 334; break; // Dec
        default:
          if (buf[0] == 'J' && buf[1] == 'a')
            daysInPrevMonths = 0;		// Jan
          else if (buf[0] == 'A' && buf[1] == 'p')
            daysInPrevMonths = 90;		// Apr
          else switch (buf[2]) {
              case 'r': daysInPrevMonths =  59; break; // Mar
              case 'y': daysInPrevMonths = 120; break; // May
              case 'n': daysInPrevMonths = 151; break; // Jun
              case 'l': daysInPrevMonths = 181; break; // Jul
              default: // add a default label here to avoid compiler warning
              case 'g': daysInPrevMonths = 212; break; // Aug
            }
      }

      // This code will not work after February 2100
      // because it does not account for 2100 not being a leap year and because
      // we use the day variable as accumulator, which would overflow in 2149
      day += (year - 1970) * 365;	// days from 1970 to the whole past year
      day += (year - 1969) >> 2;	// plus one day per leap year
      day += daysInPrevMonths;	// plus days for previous months this year
      if (daysInPrevMonths >= 59	// if we are past February
          && ((year & 3) == 0)) { 	// and this is a leap year
        day += 1;		// add one day
      }
      // Remove today, add hours, minutes and seconds this month
      time = (((day - 1ul) * 24 + hour) * 60 + minute) * 60 + second;
      setTime(time);
      timeIsSet = true;
    }
  }
}
