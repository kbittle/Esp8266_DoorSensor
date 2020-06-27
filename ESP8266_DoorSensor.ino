#include <ESP8266WiFi.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#define DEV_NAME    "ESP8266 Door Sensor"
#define SW_VERSION  "1.0.0.1"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

// Definitions
#define BATTERY_PIN A0
#define INTERVAL 5000 // 5 sec delay between publishing

// Your WiFi credentials.
// Set password to "" for open networks.
const char* ssid = "XXXXXXXX";
const char* pass = "XXXXXXX";
const char* mqtt_server = "XXXXXXXX";
const char* mqtt_login = "XXXXXXXX";
const char* mqtt_pw = "XXXXXXX";

// MQTT libraries
WiFiClient espClient;
PubSubClient client(espClient);

// Global Vars
IPAddress ip(10,0,0,50); 
IPAddress subnet(255,255,255,0);   
IPAddress gateway(10,0,0,254);
float battVoltage = 0;

// MQTT topics                                                // Expected format
#define MQTT_TOPIC_ACTIVE    "DoorSensor/State"
#define MQTT_TOPIC_WIFI_SSID "DoorSensor/Wifi/SSID"
#define MQTT_TOPIC_WIFI_IP   "DoorSensor/Wifi/LocalIP"
#define MQTT_TOPIC_WIFI_RSSI "DoorSensor/Wifi/RSSI"
#define MQTT_TOPIC_NAME      "DoorSensor/Name"
#define MQTT_TOPIC_VERSION   "DoorSensor/SwVer"
#define MQTT_TOPIC_BATT_VOLT "DoorSensor/Batt/Voltage"

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Configure static IP to reduce startup time
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//
// Description:
//  Return the quality (Received Signal Strength Indicator)
//  of the WiFi network.
//  Returns a number between 0 and 100 if WiFi is connected.
//  Returns -1 if WiFi is disconnected.
//
unsigned char getQuality() {
  if (WiFi.status() != WL_CONNECTED)
    return 0;
  int dBm = WiFi.RSSI();
  if (dBm <= -100)
    return 0;
  if (dBm >= -50)
    return 100;
  return 2 * (dBm + 100);
}

//
// Description:
//  Function handles MQTT messages. Parses the command, reactes, 
//  and loops back command in MQTT STATE field.
//
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // We dont process anything for this device
}

void reconnect() 
{
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "ESP8266DoorSensor";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_login, mqtt_pw))
    {
      Serial.println("Connected to MQTT Server");
      
      // Once connected, Publish connection data over MQTT
      client.publish(MQTT_TOPIC_WIFI_SSID, (char*) WiFi.SSID().c_str());
      client.publish(MQTT_TOPIC_WIFI_IP,   (char*) WiFi.localIP().toString().c_str());
      client.publish(MQTT_TOPIC_WIFI_RSSI, (char*) String(WiFi.RSSI()).c_str());
      client.publish(MQTT_TOPIC_NAME,      DEV_NAME);
      client.publish(MQTT_TOPIC_VERSION,   SW_VERSION);
      client.publish(MQTT_TOPIC_BATT_VOLT, String(battVoltage).c_str());

      // Sync current state with MQTT
      client.publish(MQTT_TOPIC_ACTIVE, "ON");
      
      // Delay to allow MQTT messages to TX
      delay(100);

      // Sleep until we get woken up
      ESP.deepSleep(0);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");

  // Init output pins
  pinMode(LED_BUILTIN, OUTPUT);

  // Keep wemos led off
  digitalWrite(LED_BUILTIN, HIGH);
  
  setup_wifi();
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT server
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{  
  // Calculate battery voltage
  int pinValue = analogRead(BATTERY_PIN);
  //battVoltage = pinValue * (4.2 / 1024.0); // 4.2 is the nominal voltage of the 18560 battery
  battVoltage = pinValue * (4.2 / 925);

  if (!client.connected()) 
  {
    reconnect();
  }
  
  client.loop();
}
