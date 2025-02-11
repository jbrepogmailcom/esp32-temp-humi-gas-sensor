// compile for WemosD1

// Device name and firmware version
char* D_Name = "air_sensor_jb4";
const long FW_VERSION = 20120301;

// Humidifier usage flag
int use_humidifier = 1;  // set to 0 if no humidifier

// IFTTT key
#define IFTTT_key "YOUR_IFTTT_KEY"

// Sensor data corrections
float temp_correction = -1.4;
float humi_correction = 8.30258303;
float humi_factor = 1.10701107;
bool first_run = true;

// Include necessary libraries
#include <Ticker.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "CTBot.h"
#include "DHTesp.h"
#include <MiCS6814-I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClientSecureBearSSL.h>
#include <BME280I2C.h>
#include <Wire.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <PubSubClient.h>

// Initialize objects for various functionalities
CTBot myBot;
WiFiClient w_client;
PubSubClient client(w_client);
MiCS6814 sensor;
bool sensorConnected;

// Define pin constants
#define SERIAL_BAUD 115200
#define LED_ESP 2
#define POWER_CYCLE 6
#define WD_RESET 5
#define AFTER_OTA 4
#define SDA_PIN 13  //for BME280  not used. always 4
#define SCL_PIN 12  // not used, always 5

// MQTT server details
#define mqtt_server "YOUR_MQTT_SERVER"
#define mqtt_user "YOUR_MQTT_USER"
#define mqtt_password "YOUR_MQTT_PASSWORD"

// MQTT topics
#define humidity_topic "gas_sensor_jb4/humidity"
#define temperature_topic "gas_sensor_jb4/temperature"
#define co_topic "gas_sensor_jb4/co"
#define no2_topic "gas_sensor_jb4/no2"
#define nh3_topic "gas_sensor_jb4/nh3"
#define c3h8_topic "gas_sensor_jb4/c3h8"
#define c4h10_topic "gas_sensor_jb4/c4h10"
#define ch4_topic "gas_sensor_jb4/ch4"
#define h2_topic "gas_sensor_jb4/h2"
#define c2h5oh_topic "gas_sensor_jb4/c2h5oh"

// Gas limits
#define LIMIT_CO 50
#define LIMIT_NO2 5
#define LIMIT_NH3 50
#define LIMIT_C3H8 20000
#define LIMIT_C4H10 10000
#define LIMIT_CH4 10000
#define LIMIT_C2H5OH 1000

// IFTTT parameters
String last_humi_g = "off";
const uint8_t fingerprint[20] = {0x24, 0x8E, 0x25, 0x32, 0x85, 0xE5, 0xE8, 0x64, 0x2C, 0xC9, 0x94, 0x41, 0x1A, 0xB2, 0xFB, 0x84, 0x36, 0x2B, 0xDD, 0x09};

// DHT sensor parameters
int cycleCount = 10;
int currentCycle = cycleCount + 1;
String APIkey = "YOUR_API_KEY";
String APIkeyGasses = "YOUR_API_KEY_GASSES";
const char* TSserver = "api.thingspeak.com";
int DHTIO = 13;
int DHTGround = 12;
String st = "";
unsigned int recipient;
unsigned long timerdelay = 3*60*1000;
unsigned long lastreset = 0;

// Toshiba AC control parameters
#include <IRremoteESP8266.h>  //https://github.com/crankyoldgit/IRremoteESP8266
#include <IRsend.h>
#include <ir_Toshiba.h>
#include <string.h>
const uint16_t kIrLed = 14;  // ESP8266 GPIO pin 14 D5 to use. Recommended: 4 (D2).
IRToshibaAC ac(kIrLed);

// Function to print AC state
void printState(unsigned int my_telegram_id) {
  Serial.println("Toshiba A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  myBot.sendMessage(my_telegram_id, ac.toString().c_str());
  myBot.sendMessage(my_telegram_id, "Temp: " + st + " (v." + FW_VERSION + ")");
  unsigned char* ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kToshibaACStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

// Firmware update parameters
const char* fwUrlBase = "DIRECTORY";
const char* fwUrlServer = "SERVER IP";
const char* fwUrlPath = "/files/";
const int fwUrlPort = 80;
String OldCommand = "";
String newCommand = "";

// IFTTT event parameters
String IFTTT_event = "temp-humi";
String IFTTT_url1 = "https://maker.ifttt.com/trigger/";
String IFTTT_url2 = "/with/key/";
float altitude_m = 289.0;
float temp_thresh1 = 23.0;
float temp_thresh2 = 26.0;
float temp_thresh3 = 32.0;
float humi_thresh1 = 45.0;
String t_eventname = "";
String h_eventname = "";
String old_t_eventname = "";
String old_h_eventname = "";
String IFTTT_url = "";

// Telegram bot token and ID
String token = "YOUR_TELEGRAM_BOT_TOKEN";
unsigned int my_telegram_id = 12345678;
unsigned int sleep_time = 600000000;
rst_info *xyz;
byte reset_reason;
int valid = 0;
Ticker ticker;
String ssid = "";
String password = "";

// ADC_MODE(ADC_VCC);

// Pin definitions
#define LED_ESP 2
#define PH_VCC 14
#define PH_GND 12
#define THRESHOLD 900
#define POWER_CYCLE 6
#define WD_RESET 5
#define AFTER_OTA 4
#define BUZZER 16

// BME280 sensor object
BME280I2C bme;

// Setup function
void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize MQTT client
  client.setServer(mqtt_server, 1883);

  // Initialize buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Initialize Toshiba AC control
  pinMode(LED_BUILTIN, OUTPUT);
  ac.begin();
  delay(200);
  ac.on();
  ac.setMode(kToshibaAcAuto);
  ac.setTemp(25);

  // Initialize MiCS-6814 sensor
  delay(3000);
  sensorConnected = sensor.begin();
  if (sensorConnected) {
    Serial.println("Connected to MiCS-6814 sensor");
    sensor.powerOn();
    Serial.println("Current concentrations:");
    Serial.println("CO\tNO2\tNH3\tC3H8\tC4H10\tCH4\tH2\tC2H5OH");
  } else {
    Serial.println("Couldn't connect to MiCS-6814 sensor");
  }

  // Initialize LED
  pinMode(LED_ESP, OUTPUT);
  Serial.println("Starting...");

  // Check reset reason
  xyz = ESP.getResetInfoPtr();
  reset_reason = (*xyz).reason;
  Serial.println(reset_reason);

  // Connect to WiFi
  connect_to_wifi_jb();
  Serial.println("connected.");

  // Initialize Telegram bot
  myBot.setTelegramToken(token);
  if (myBot.testConnection())
    Serial.println("\nBOT testConnection OK");
  else
    Serial.println("\nBOT testConnection NOK");

  myBot.sendMessage(my_telegram_id, "Tady AC, boot, kontroluji update...");
  checkForUpdates2();

  // Initialize BME280 sensor
  Wire.begin();
  while(!bme.begin()) {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }
  switch(bme.chipModel()) {
    case BME280::ChipModel_BME280:
      Serial.println("Found BME280 sensor! Success.");
      break;
    case BME280::ChipModel_BMP280:
      Serial.println("Found BMP280 sensor! No Humidity available.");
      break;
    default:
      Serial.println("Found UNKNOWN sensor! Error!");
  }

  // Send first measurement after reset
  float temp(NAN), hum(NAN), pres(NAN);
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  bme.read(pres, temp, hum, tempUnit, presUnit);
  delay(1000);
  bme.read(pres, temp, hum, tempUnit, presUnit);
  pres = pres/100 + altitude_m/8.3;

  //########### Send IFTTT events ############

  /* std::unique_ptr<BearSSL::WiFiClientSecure>w_client(new BearSSL::WiFiClientSecure);
  w_client->setFingerprint(fingerprint);

  w_client->print("Temp: ");
  w_client->print(temp);
  w_client->print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? 'C' :'F'));
  w_client->print("\t\tHumidity: ");
  w_client->print(hum);
  w_client->print("% RH");
  w_client->print("\t\tPressure: ");
  w_client->print(pres);
  w_client->println(" hPa");

  Serial.print("[HTTPS] begin...\n");

  HTTPClient https;

  String IFTTT_values = "?value1=" + String(temp) + "&value2=" + String(hum) + "&value3=" + String(pres);
  String IFTTT_url = IFTTT_url1 + IFTTT_event + IFTTT_url2 + IFTTT_key + IFTTT_values;  
  Serial.println("Sending IFTTT url: "+IFTTT_url);
  
  if (https.begin(*w_client, IFTTT_url)) {  // HTTPS

    Serial.print("[HTTPS] GET...\n");
    // start connection and send HTTP header
    int httpCode = https.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  } */

  //debugReadMICS(367);

}

//################################### START ###############################################xx

// Loop function
void loop() {
  // Read temperature and humidity every 50 cycles (5 minutes)
  currentCycle++;
  if (currentCycle >= cycleCount) {
    float temp(NAN), hum(NAN), pres(NAN);
    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);
    bme.read(pres, temp, hum, tempUnit, presUnit);
    delay(1000);
    bme.read(pres, temp, hum, tempUnit, presUnit);
    pres = pres/100 + altitude_m/8.3;

    float temperature = temp;
    float humidity = hum;

    // Determine IFTTT event names based on thresholds
    if (temp < temp_thresh1) t_eventname = "temp-low";
    if ((temp >= temp_thresh1) && (temp < temp_thresh2)) t_eventname = "temp-lm";
    if ((temp >= temp_thresh2) && (temp < temp_thresh3)) t_eventname = "temp-hm";
    if (temp >= temp_thresh3) t_eventname = "temp-high";
    if (hum < humi_thresh1) h_eventname = "humi-low";
    if (hum >= humi_thresh1) h_eventname = "humi-high";

    //########### Send IFTTT events ############
    /* 
    std::unique_ptr<BearSSL::WiFiClientSecure>w_client(new BearSSL::WiFiClientSecure);
    w_client->setFingerprint(fingerprint);
  
    HTTPClient https;
    
    if (old_t_eventname != t_eventname) {
  
      Serial.print("[HTTPS] begin...\n");
      
      IFTTT_url = IFTTT_url1 + t_eventname + IFTTT_url2 + IFTTT_key;  
    
      Serial.println("Sending IFTTT url: "+IFTTT_url);
      
      if (https.begin(*w_client, IFTTT_url)) {  // HTTPS
    
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();
    
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
    
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
    
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
    }  

    if (old_h_eventname != h_eventname) {
    
      Serial.print("[HTTPS] begin...\n");
    
      IFTTT_url = IFTTT_url1 + h_eventname + IFTTT_url2 + IFTTT_key;  
    
      Serial.println("Sending IFTTT url: "+IFTTT_url);
      
      if (https.begin(*w_client, IFTTT_url)) {  // HTTPS
    
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();
    
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
    
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
    
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
    }  */

    old_t_eventname = t_eventname;
    old_h_eventname = h_eventname;

    // Print temperature and humidity
    Serial.println(humidity);
    Serial.println(temperature);
    st = String(temperature);
    String sh = String(humidity);
    Serial.println("Temp: " + st + ", Humi: " + sh);
    currentCycle = 0;

    // Control humidifier based on humidity
    check_humidity(humidity);

    // Get gas measurements if sensor is connected
    if (sensorConnected) {
      // Print live values
      /*
      Serial.print(sensor.measureCO());
      Serial.print("\t");
      Serial.print(sensor.measureNO2());
      Serial.print("\t");
      Serial.print(sensor.measureNH3());
      Serial.print("\t");
      Serial.print(sensor.measureC3H8());
      Serial.print("\t");
      Serial.print(sensor.measureC4H10());
      Serial.print("\t");
      Serial.print(sensor.measureCH4());
      Serial.print("\t");
      Serial.print(sensor.measureH2());
      Serial.print("\t");
      Serial.println(sensor.measureC2H5OH());
      */ 
    }

    // Log data to ThingSpeak and MQTT
    if (first_run || ((millis() - lastreset) > timerdelay)) {
      logTSGasses(sensor.measureCO(), sensor.measureNO2(), sensor.measureNH3(), sensor.measureC3H8(), sensor.measureC4H10(), sensor.measureCH4(), sensor.measureH2(), sensor.measureC2H5OH());
      logTS(sh, st, pres);
      lastreset = millis();
      first_run = false;
    }
  }

/*
  // Read CTbot message
  TBMessage msg;
  String tms = "";
  int cmdsent = 0;

  recipient = my_telegram_id;

  readAcServer();

  //Serial.print("Command retrieved: ");
  //Serial.println(newCommand);

  if (newCommand != OldCommand) {
    tms = newCommand;
    Serial.print("Preparing to send command from server: ");
    Serial.println(tms);
    myBot.sendMessage(recipient, "Server command: " + newCommand);
  }
  OldCommand = newCommand;

  // if there is an incoming message...
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1);
  digitalWrite(LED_BUILTIN, HIGH);
  if (myBot.getNewMessage(msg)) {
    // ...forward it to the sender
    cmdsent = 1;
    tms = msg.text;
    tms.toUpperCase();
    recipient = msg.sender.id;
    myBot.sendMessage(recipient, "Příjem: " + tms);
  }

  valid = 0;

  char tm[50];

  tms.toCharArray(tm, 50);
  char* word1 = strtok(tm, " ");
  char* word2 = strtok(NULL, " ");
  String w1 = String(word1);
  String w2 = String(word2);
  //Serial.println("Word1: " + w1 + "; Word2: " + w2);

  // Now send the IR signal.
#if SEND_TOSHIBA_AC

  if (w1 == "POWER") {
    if (w2 == "ON") {
      ac.on();
      valid = 1;
      // OldCommand = "POWER ON";
    }
    if (w2 == "OFF") {
      ac.off();
      valid = 1;
      // OldCommand = "POWER OFF";
    }
  }
  if (w1 == "MODE") {
    if (w2 == "AUTO") {
      ac.setMode(kToshibaAcAuto);
      valid = 1;
    }
    if (w2 == "COOL") {
      ac.setMode(kToshibaAcCool);
      valid = 1;
    }
    if (w2 == "HEAT") {
      ac.setMode(kToshibaAcHeat);
      valid = 1;
    }
  }

  if (w1 == "FAN") {
    if (w2 == "AUTO") {
      ac.setFan(kToshibaAcFanAuto);
      valid = 1;
    }
    if (w2 == "MIN") {
      ac.setFan(1);
      valid = 1;
    }
    if (w2 == "MAX") {
      ac.setFan(kToshibaAcFanMax);
      valid = 1;
    }
  }

  if (w1 == "TEMP") {
    int tem = w2.toInt();
    ac.setTemp(tem);
    valid = 1;
  }

  if ((w1 == "RESET") || (w1 == "REBOOT")) {
    myBot.sendMessage(my_telegram_id, "REBOOT!");
    w1 = "";
    //ESP.restart();
  }


  if (valid == 1) {
    Serial.println("Sending IR command to A/C ...");
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);
    ac.send();
    digitalWrite(LED_BUILTIN, HIGH);
#endif  // SEND_TOSHIBA_AC 
    printState(my_telegram_id);
    //myBot.sendMessage(my_telegram_id, "Příkaz poslán.");
  }
  else if (cmdsent == 1) myBot.sendMessage(recipient, "Neznámý příkaz!");
*/
  delay(5000);

}

//############################## STOP ##################################xx

// Function to debug read MICS sensor
void debugReadMICS(int line) {
  Serial.println("Debug at line " + String(line));
  if (sensorConnected) {
    // Print live values
    Serial.print(sensor.measureCO());
    Serial.print("\t");
    Serial.print(sensor.measureNO2());
    Serial.print("\t");
    Serial.print(sensor.measureNH3());
    Serial.print("\t");
    Serial.print(sensor.measureC3H8());
    Serial.print("\t");
    Serial.print(sensor.measureC4H10());
    Serial.print("\t");
    Serial.print(sensor.measureCH4());
    Serial.print("\t");
    Serial.print(sensor.measureH2());
    Serial.print("\t");
    Serial.println(sensor.measureC2H5OH());
  }
}

// Function to check humidity and control humidifier
void check_humidity(float humi) {
/*
  #define h_off "humidifier-off"
  #define h_on "humidifier-on"

  //String last_humidity = "undefined";
  //String h_command = "humidifier-off";

  Serial.println("Checking if difuser enabled...");
  if (use_humidifier == 1) {
    Serial.println("...it is!");
    if ((humi < 40) && (last_humi_g == "off")) {
      Serial.println("Turning webhook ON");
      send_IFTTT_event("humidifier-on");
      last_humi_g="on";
    }

    if ((humi > 50) && (last_humi_g == "on")) {
      Serial.println("Turning webhook OFF");
      send_IFTTT_event("humidifier-off");
      last_humi_g="off";
    }
  }
*/
}

// Function to log data to ThingSpeak
void logTS(String param1, String param2, float nparam3) {
  int cc = w_client.connect(TSserver, 80);
  if (cc) {
    // vytvoření zprávy, která bude odeslána na Thingspeak
    // každé pole je označeno jako "field" + pořadí pole,
    // je nutné každý údaj převést na String
    String zprava = APIkey;
    zprava += "&field1=";
    zprava += param1;
    zprava += "&field2=";
    zprava += param2;
    zprava += "&field5=";
    zprava += String(nparam3);
    zprava += "\r\n\r\n";
    // po vytvoření celé zprávy ji odešleme na server Thingspeak
    // včetně našeho API klíče
    w_client.print("POST /update HTTP/1.1\n");
    w_client.print("Host: api.thingspeak.com\n");
    w_client.print("Connection: close\n");
    w_client.print("X-THINGSPEAKAPIKEY: " + APIkey + "\n");
    w_client.print("Content-Type: application/x-www-form-urlencoded\n");
    w_client.print("Content-Length: ");
    w_client.print(zprava.length());
    w_client.print("\n\n");
    w_client.print(zprava);
    // vytištění informací po sériové lince o odeslání na Thingspeak
    Serial.print("param1: ");
    Serial.print(param1);
    Serial.print(" param2: ");
    Serial.println(param2);
    Serial.print(" param5: ");
    Serial.println(nparam3);
    Serial.println("Udaje odeslany na Thingspeak.");
  }
  else {
    //ESP.restart();
    Serial.println();
    Serial.println("#### ThingSpeak API can not connect ####");
  }
  // ukončení spojení se serverem Thingspeak

  w_client.stop();
  
  Serial.println("Nyni odesilam mqtt - temp+humi...");
  int i = 10;
  while (!client.connected() && (i >=0)) {
    i--;
    reconnect();
    Serial.print(".");
    delay(1000);
    }
  if (i >=1) {    
    client.loop();
    client.publish(temperature_topic, param2.c_str(), true);
    delay(300);
    client.publish(humidity_topic, param1.c_str(), true);
    delay(300);
    Serial.println("...mqtt odeslano.");
    }
  else {
    Serial.println("...nelze spojit na mqtt.");  
    }
}

// Function to reconnect to MQTT server
void reconnect() {
  // Loop until we're reconnected
  int i = 10;
  Serial.print("Attempting MQTT connection...");
  while (!client.connected() && (i >= 0 )) {
    i--;
    Serial.print(".");
    if (client.connect(D_Name, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      }
    if (i <= 0) {
      Serial.println("Could not connect to mqtt.");  
      }
    }
}

// Function to log gas data to ThingSpeak
void logTSGasses(float g1, float g2, float g3, float g4, float g5, float g6, float g7, float g8) {
  int cc = w_client.connect(TSserver, 80);
  if (cc) {
    // vytvoření zprávy, která bude odeslána na Thingspeak
    // každé pole je označeno jako "field" + pořadí pole,
    // je nutné každý údaj převést na String
    String zprava = APIkeyGasses;
    zprava += "&field1=";
    zprava += String(g1);
    zprava += "&field2=";
    zprava += String(g2);
    zprava += "&field3=";
    zprava += String(g3);
    zprava += "&field4=";
    zprava += String(g4);
    zprava += "&field5=";
    zprava += String(g5);
    zprava += "&field6=";
    zprava += String(g6);
    zprava += "&field7=";
    zprava += String(g7);
    zprava += "&field8=";
    zprava += String(g8);

    Serial.print(g1);
    Serial.print("\t");
    Serial.print(g2);
    Serial.print("\t");
    Serial.print(g3);
    Serial.print("\t");
    Serial.print(g4);
    Serial.print("\t");
    Serial.print(g5);
    Serial.print("\t");
    Serial.print(g6);
    Serial.print("\t");
    Serial.print(g7);
    Serial.print("\t");
    Serial.print(g8);
    Serial.println("\t");
    
    zprava += "\r\n\r\n";
    // po vytvoření celé zprávy ji odešleme na server Thingspeak
    // včetně našeho API klíče
    w_client.print("POST /update HTTP/1.1\n");
    w_client.print("Host: api.thingspeak.com\n");
    w_client.print("Connection: close\n");
    w_client.print("X-THINGSPEAKAPIKEY: " + APIkeyGasses + "\n");
    w_client.print("Content-Type: application/x-www-form-urlencoded\n");
    w_client.print("Content-Length: ");
    w_client.print(zprava.length());
    w_client.print("\n\n");
    w_client.print(zprava);
  }
  else {
    //ESP.restart();
  }
  // ukončení spojení se serverem Thingspeak

  w_client.stop();

  Serial.println("Nyni odesilam mqtt - gasses...");
  if (!client.connected()) {
    reconnect();
    }
  client.loop();
  client.publish(co_topic, String(g1).c_str(), true);
  delay(300);
  client.publish(no2_topic, String(g2).c_str(), true);
  delay(300);
  client.publish(nh3_topic, String(g3).c_str(), true);
  delay(300);
  client.publish(c3h8_topic, String(g4).c_str(), true);
  delay(300);
  client.publish(c4h10_topic, String(g5).c_str(), true);
  delay(300);
  client.publish(ch4_topic, String(g6).c_str(), true);
  delay(300);
  client.publish(h2_topic, String(g7).c_str(), true);
  delay(300);
  client.publish(c2h5oh_topic, String(g8).c_str(), true);
  delay(300);
  Serial.println("...mqtt odeslano.");

}

// Function to read AC server command
void readAcServer() {
  String mac = String(D_Name);
  String fwURL = String( fwUrlBase );
  fwURL.concat( mac );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".command" );

  //Serial.println( "Checking for server command." );
  //Serial.print( "MAC address: " );
  //Serial.println( mac );
  //Serial.print( "Command URL: " );
  //Serial.println( fwVersionURL );

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  int httpCode = httpClient.GET();
  if ( httpCode == 200 ) {
    newCommand = httpClient.getString();

    //Serial.print( "Available Command: " );
    newCommand.toUpperCase();
    //Serial.println( newCommand );
    Serial.print(".");
  }
  else {
    Serial.print( "Command failed, got HTTP response code " );
    Serial.println( httpCode );
  }
  httpClient.end();
  return;
}

// Function to connect to WiFi
void connect_to_wifi_jb() {
  Serial.println("Setting WiFi connection...");
  delay(100);
  pinMode(LED_ESP, OUTPUT);
  ticker.attach(0.6, tick);
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(300);
  wifiManager.setConnectTimeout(60);
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect(D_Name)) {
    Serial.println("failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  }
  Serial.println("connected...yeey :)");
  ticker.detach();
  digitalWrite(LED_ESP, LOW);
  delay(3000);
  digitalWrite(LED_ESP, HIGH);
  Serial.print("WiFi.SSID(): ");
  Serial.println(WiFi.SSID());
  //WiFi.SSID().toCharArray(ssid, 32);
  Serial.print("WiFi.psk(): ");
  Serial.println(WiFi.psk());
  //WiFi.psk().toCharArray(password, 32);
}

// Function to handle OTA updates
void OTAtick() {
  //ArduinoOTA.handle();
}

// Function to delay with OTA handling
void delayOTA(int duration) {                       // Use this loop to keep OTA alive
  for (int i = 0; i <= duration / 1000; i++) {
    //ArduinoOTA.handle();
    delay(1000);
  }
}

// Function to blink LED
void blink_led(int which, int cas1, int cas2, int inverse) {
  pinMode(which, OUTPUT);
  digitalWrite(which, 1 - inverse);
  delay(cas1);
  digitalWrite(which, inverse);
  delay(cas2);
}

// Function to toggle LED state
void tick() {
  //toggle state
  int state = digitalRead(LED_ESP);  // get the current state of GPIO1 pin
  digitalWrite(LED_ESP, !state);     // set pin to the opposite state
}

// Callback function for WiFiManager configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

// Function to check for firmware updates
void checkForUpdates() {
  String mac = String(D_Name);
  String fwURL = String( fwUrlBase );
  fwURL.concat( mac );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );

  Serial.println( "Checking for firmware updates." );
  Serial.print( "MAC address: " );
  Serial.println( mac );
  Serial.print( "Firmware version URL: " );
  Serial.println( fwVersionURL );

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  //httpClient.begin("http://79.137.79.8/files/ESP_c_1.version");
  int httpCode = httpClient.GET();
  if ( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    Serial.print( "Current firmware version: " );
    Serial.println( FW_VERSION );
    Serial.print( "Available firmware version: " );
    Serial.println( newFWVersion );

    long newVersion = newFWVersion.toInt();

    if ( newVersion > FW_VERSION ) {
      Serial.println( "Preparing to update" );
      myBot.sendMessage(my_telegram_id, "Updating to version " + newFWVersion);
      String fwImageURL = fwURL;
      fwImageURL.concat( ".bin" );
      //ticker.attach(0.1, tick);
      ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

      WiFiClient ww_client;
      t_httpUpdate_return ret = ESPhttpUpdate.update( ww_client, fwImageURL );
      //ticker.detach();

      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          myBot.sendMessage(my_telegram_id, "HTTP_UPDATE_FAILD Error: " + ESPhttpUpdate.getLastErrorString()); //+ESPhttpUpdate.getLastError()+", "
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          myBot.sendMessage(my_telegram_id, "HTTP_UPDATE_NO_UPDATES");
          break;
      }
    }
    else {
      Serial.println( "Already on latest version" );
      myBot.sendMessage(my_telegram_id, "Already on latest version" );
    }
  }
  else {
    Serial.print( "Firmware version check failed, got HTTP response code " );
    Serial.println( httpCode );
    myBot.sendMessage(my_telegram_id, "Firmware version check failed, got HTTP response code " + String(httpCode));
  }
  httpClient.end();
}

// Function to check for firmware updates (alternative method)
void checkForUpdates2() {
  String mac = String(D_Name);
  String fwURL = String( fwUrlBase );
  fwURL.concat( mac );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );

  Serial.println( "Checking for firmware updates." );
  Serial.print( "MAC address: " );
  Serial.println( mac );
  Serial.print( "Firmware version URL: " );
  Serial.println( fwVersionURL );

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  //httpClient.begin("http://79.137.79.8/files/ESP_c_1.version");
  int httpCode = httpClient.GET();
  if ( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    Serial.print( "Current firmware version: " );
    Serial.println( FW_VERSION );
    Serial.print( "Available firmware version: " );
    Serial.println( newFWVersion );

    long newVersion = newFWVersion.toInt();

    if ( newVersion > FW_VERSION ) {
      Serial.println( "Preparing to update" );
      myBot.sendMessage(my_telegram_id, "Updating to version " + newFWVersion);
      String fwImageURL = fwURL;
      fwImageURL.concat( ".bin" );
      //ticker.attach(0.1, tick);
      ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

      //WiFiClient ww_client;
      //t_httpUpdate_return ret = ESPhttpUpdate.update( ww_client, fwImageURL );

      //httpClient.end();
      String fwUrlPath2bin = fwUrlPath;
      fwUrlPath2bin.concat(D_Name);
      fwUrlPath2bin.concat(".bin");
      Serial.print("Server: ");
      Serial.println(fwUrlServer);
      Serial.println("Path:   "+fwUrlPath2bin);
      //t_httpUpdate_return ret = ESPhttpUpdate.update("SERVER IP", 80, "/files/toshiba_ac.bin");
      t_httpUpdate_return ret = ESPhttpUpdate.update(fwUrlServer, fwUrlPort, fwUrlPath2bin);
      
      //ticker.detach();

      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          myBot.sendMessage(my_telegram_id, "HTTP_UPDATE_FAILD Error: " + ESPhttpUpdate.getLastErrorString()); //+ESPhttpUpdate.getLastError()+", "
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          myBot.sendMessage(my_telegram_id, "HTTP_UPDATE_NO_UPDATES");
          break;
      }
    }
    else {
      Serial.println( "Already on latest version" );
      myBot.sendMessage(my_telegram_id, "Already on latest version" );
    }
  }
  else {
    Serial.print( "Firmware version check failed, got HTTP response code " );
    Serial.println( httpCode );
    myBot.sendMessage(my_telegram_id, "Firmware version check failed, got HTTP response code " + String(httpCode));
  }
  httpClient.end();
}


/*
  String getMAC()
  {
  uint8_t mac[6];
  char result[14];

  snprintf( result, sizeof( result ), "%02x%02x%02x%02x%02x%02x", mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ] );

  return String( result );
  }

  int getLght() {

  pinMode(PH_VCC, OUTPUT);
  pinMode(PH_GND, OUTPUT);
  digitalWrite(PH_VCC, HIGH);
  digitalWrite(PH_GND, LOW);

  //int Light = 1000; //analogRead(A0);
  int Light = analogRead(A0);
  Serial.print("Light: ");
  Serial.println(Light);
  delay(50);
  pinMode(PH_VCC, INPUT);
  pinMode(PH_GND, INPUT);

  return Light;
  }
*/
