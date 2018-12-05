#include <SPI.h>
#include <WiFi.h>
#include "DHT.h"
#include <Timer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include "wiring_watchdog.h"
#define pinStep 8
#define pinDirection 9
#define DHTTYPE DHT11
#define DHTPIN 2  

int getdata_time = 1000; // time intervals to get data from thingspeak 
Timer getdata;

char ssid[] = "D-School_IARC";      //  your network SSID (name) 
char pass[] = "IARC0324";   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)

char chnnel_num[] = "206494";  //your public chnnel
char cheenl_field[] ="4";  //your chnnel field

int status = WL_IDLE_STATUS;
DHT dht(DHTPIN, DHTTYPE);
WiFiServer server(80);
int pin_fire=3;//設定火焰感測器接腳在第3孔
int val_fire;//設定變量val_fire
int rightspin;//馬達右轉
int leftspin;//馬達左轉

// LCM1602 I2C LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
AccelStepper mystepper(1, pinStep, pinDirection); //accel stepper 

// ThingSpeak Settings
// server address:
char thingSpeakAddress[] = "api.thingspeak.com";
String APIKey = "SBPCASN0S25F8NVX"; //nter your channel's Write API Key
const int updateThingSpeakInterval = 20 * 1000; // 20 second interval at which to update ThingSpeak

// Variable Setup
long lastConnectionTime = 0;
boolean lastConnected = false;

// Initialize Arduino Ethernet Client
WiFiClient client;

void setup() {
  getdata.every(getdata_time, getdata_thingspeak); // start timer
  // Start Serial for debugging on the Serial Monitor
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  dht.begin();
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv != "1.1.0") {
    Serial.println("Please upgrade the firmware");
  }
  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  // you're connected now, so print out the status:
  server.begin();
  printWifiStatus();
  Serial.println("DHTxx test!");
  dht.begin();
  pinMode(pin_fire,INPUT);//設定火焰感測器為输入
  lcd.begin(16,2);               // initialize the lcd 
  lcd.backlight();
  pinMode(pinStep,OUTPUT) ;
  pinMode(pinDirection,OUTPUT) ;
}

void loop() {
  delay(2000);
  getdata.update();
 // Wait a few seconds between measurements.
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  lcd.clear();
  lcd.setCursor ( 0, 0 );        // go to home
  lcd.print("Humid: ");
  lcd.print(h);
  lcd.print(" %"); 
  lcd.setCursor ( 0, 1 );        // go to the next line
  lcd.print("Temp : ");
  lcd.print(t);
  lcd.print(" *C ");
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humid: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temp : ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.print(hif);
  Serial.println(" *F");

  // Print Update Response to Serial Monitor
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
  // Disconnect from ThingSpeak
  if (!client.connected() && lastConnected) {
    Serial.println("...disconnected");
    Serial.println();
    client.stop();
  }
  // Update ThingSpeak
  val_fire=digitalRead(pin_fire);//將火焰感測器的值讀取给val
  if (!client.connected() && (millis() - lastConnectionTime > updateThingSpeakInterval)) {
    if(val_fire == HIGH){
      updateThingSpeak("field1=" +String(t)+"&field2="+String(h)+"&field3="+1+"&field4="+0);
     }
     else{
      updateThingSpeak("field1=" +String(t)+"&field2="+String(h)+"&field3="+0+"&field4="+0);
     }
  }
  lastConnected = client.connected();
}

void updateThingSpeak(String tsData) {
  if (client.connect(thingSpeakAddress, 80)) {
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + APIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");
    client.print(tsData);
    lastConnectionTime = millis();

    if (client.connected()) {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
    }
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


void getdata_thingspeak(){
int t = 0;
    if( !client.connect( thingSpeakAddress, 80 ) )
    {
      Serial.println( "connection failed" );
      wdt_enable(10);
      delay(1000);
    }
    else
    {
      //client.println("GET /channels/XXXXX/fields/XXXXX/last.txt HTTP/1.0");
      client.print("GET /channels/");
      client.print(chnnel_num);
      client.print("/fields/");
      client.print(cheenl_field);
      client.print("/last.txt HTTP/1.0");      
      client.println();
      client.println("Host: api.thingspeak.com");
      client.println("User-Agent: arduino-ethernet");
      client.println("Connection: close");
      client.println();
      delay(1000);
        while (client.available()) {
          char c = client.read();
          Serial.print(c);
          mystepper.setMaxSpeed(400);
          mystepper.setAcceleration(400);
          if(c == '1')
          {
            Serial.print(100);
            //mystepper.moveTo(5);
             mystepper.moveTo(mystepper.currentPosition()+5);
             mystepper.runToPosition();
          }
          if(c == '2')
          {
            Serial.print(200);
            //mystepper.moveTo(-5);
             mystepper.moveTo(mystepper.currentPosition()-5);
            mystepper.runToPosition();
          }
        }
    }
    client.stop();
}        
