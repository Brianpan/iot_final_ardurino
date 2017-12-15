#include <SoftwareSerial.h> 
#include <stdlib.h>
#include <Adafruit_GPS.h>
#include "SparkFunLSM6DS3.h"

// ESP 8266
// RX ESP -> TX Arduino
// TX ESP -> RX Arduino
SoftwareSerial ESPSerial(4, 5); // RX, TX

String req;
// data
String data;
String lng;
String lat;
float fLng;
float fLat;
char char_f[9];

// GPS Sensor
SoftwareSerial mySerial(3, 2);

Adafruit_GPS GPS(&mySerial);
#define GPSECHO  false

// this keeps track of whether we're using the interrupt
// off by default!
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

// acc sensor
LSM6DS3 myIMU;

// this runs once
void setup() {                
 
  // enable debug serial
  Serial.begin(9600); 
  // enable software serial
  ESPSerial.begin(9600);
 
  // reset ESP8266
  ESPSerial.println("AT+RST");
  
  //conect to network
  ESPSerial.println("AT+CWMODE=3");
  ESPSerial.println("AT+CWJAP=\"brian\",\"12345678\"");
  delay(4000);
  // set API
  setAPI();
  
  // GPS settings
  gpsSetup();
  
  if( myIMU.beginCore() != 0 )
  {
    Serial.print("Error at beginCore().\n");
  }
  else
  {
    Serial.print("\nbeginCore() passed.\n");
  }
  myIMU.begin();
  Serial.println("Setting up finish");
}

void gpsSetup(){
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  useInterrupt(false);
}

void setAPI(){
  // TCP connection
  ESPSerial.println("AT+CIPMUX=0"); 
}

void sendData(){
  ESPSerial.println("AT+CIPSTART=\"TCP\",\"104.200.18.210\",3000");
   
  if(ESPSerial.find("Error")){
    Serial.println("AT+CIPSTART error");
    delay(2000);
    setAPI();
  }
  else{
    Serial.println("TCP success");
  }
  delay(3000);
  req = "";
  data = "lng=" + lng +
         "&lat=" + lat +
         "&device_id=1";
  int len = 0;
  req =  req + "POST /sensors HTTP/1.1" + "\r\n" + 
        "HOST: 104.200.18.210" + "\r\n" +
        "Accept: *" + "/" + "*\r\n" +
        "Content-Length: " + data.length() + "\r\n" + 
        "Content-Type: application/x-www-form-urlencoded\r\n" +
        "\r\n" + data;

//  Serial.println(req.length());
//  Serial.println(req);
  ESPSerial.print("AT+CIPSEND=");
  ESPSerial.println(req.length());
  
  if(ESPSerial.find(">"))
  {
    Serial.println(req.length());
    Serial.println(req);
    ESPSerial.println(req);
  }
  else
  {
    ESPSerial.println("AT+CIPCLOSE");
    Serial.println("Sent failed!");
  }
  
  if( ESPSerial.find("OK") )
  {
    Serial.println( "RECEIVED: OK" );
  }
  ESPSerial.println("AT+CIPCLOSE");
  delay(60000);
}


void getAccel() {
//  for debug
//  Serial.print("x:");
//  Serial.println(x,4);
//  Serial.print("y:");
//  Serial.println(y,4);
//  Serial.print("z:");
//  Serial.println(z,4);
  // to detect is falling or not
  if(myIMU.readFloatAccelZ() >= 2 || myIMU.readFloatAccelX() >= 1 || myIMU.readFloatAccelY()>= 1){
    Serial.println("Detect Fail");
    sendData();
  }
}

// gps utils
// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
#ifdef UDR0
  if (GPSECHO){
    if (c){
      UDR0 = c;
    }
  }
  //delay(2000);
#endif
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

uint32_t timer = millis();

void fetchGps(){
  // need to 'hand query' the GPS, not suggested :(
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
  }
  
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis())  timer = millis();
  if (millis() - timer > 30000)
    timer = millis();
    
  if (GPS.fix) {
      fLat = GPS.latitudeDegrees;
      fLng = GPS.longitudeDegrees;

      dtostrf(fLat, 8, 4, char_f);
      lat = "";
      lng = "";
      for(int i =0; i<sizeof(char_f);i++){
        if(char_f[i] != " ")
          lat += char_f[i];
      }
      lat.replace(" ","");
      dtostrf(fLng, 8, 4, char_f);
      for(int i =0; i<sizeof(char_f);i++){
        lng += char_f[i];
      }
  }
  Serial.print(lng);
  Serial.println(" " + lat);
}
// the loop 
void loop() {
  getAccel();
  fetchGps();
  delay(1000);
}
