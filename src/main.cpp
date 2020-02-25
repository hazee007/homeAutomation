#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include "FS.h"      
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          
#include <ArduinoJson.h> 


WiFiClient espClient;
PubSubClient client(espClient);

#define relayPin D2
#define resetPin D5


DHTesp dht;

int buttonState = 0;     
int lastButtonState = 0; 
int startPressed = 0;    
int endPressed = 0;      
int holdTime = 0;        
String Mac, temperature, humidity, HeatIndex;


long lastSent = 0;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "1883";
char device_name[40];


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//callback for MQTT..
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(relayPin, LOW);   
  } else {
    digitalWrite(relayPin, HIGH); 
  }

}


void Ondemand(){
  WiFiManager wifiManager;

    if (!wifiManager.startConfigPortal("OnDemandAP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("WiFi connected....");
}

// BUTTON STATES
void updateState() {
  // the button has been just pressed
  if (buttonState == LOW) {
      startPressed = millis();
  // the button has been just released
  } else {
      endPressed = millis();
      holdTime = endPressed - startPressed;

      if (holdTime > 100 && holdTime < 500) {
          Ondemand(); 
      }

      else if (holdTime >= 500 && holdTime <= 1500) {
          WiFi.disconnect(true);
          ESP.restart();
      }

  }
}


void Reset(){
  buttonState = digitalRead(resetPin);
  if (buttonState != lastButtonState) { 
    lastButtonState = buttonState;
     updateState();
  }
}


//MQTT Reconnect...
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
      //   WiFi.disconnect(true);
      //  ESP.restart();
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish device details...
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  dht.setup(D3, DHTesp::DHT11);
  pinMode(relayPin, OUTPUT);
  pinMode(resetPin, INPUT_PULLUP);
  digitalWrite(relayPin, HIGH);

  //clean FS, for testing 
  // SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
        auto error = deserializeJson(json, buf.get());
        serializeJson(json, Serial);

          if (!error ) {
          Serial.println("\nparsed json");

          strlcpy(mqtt_server, json["mqtt_server"]| "N/A",40);
          strlcpy(mqtt_port, json["mqtt_port"]| "N/A",6);
          strlcpy(device_name, json["device_name"]| "N/A",40);
        }
        else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_device_name("device", "device_name", device_name, 40);
    

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

  
    

    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    
    //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_device_name);

    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("WiFi connected......");

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(device_name, custom_device_name.getValue());
    
    
    //save the custom parameters to FS
    if (shouldSaveConfig) {
      Serial.println("saving config");
      DynamicJsonDocument json(1024);
      json["mqtt_server"] = mqtt_server;
      json["mqtt_port"] = mqtt_port;
      json["device_name"] = device_name;
      
      
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.println("failed to open config file for writing");
      }

      serializeJson(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      //end save
    }
    int port = atoi(mqtt_port);

    client.setServer(mqtt_server, port);
    client.setCallback(callback);
    Mac = WiFi.macAddress();

}


void TempHum()
{
  float hum = dht.getHumidity();
  float temp = dht.getTemperature();

  temperature = temp;
  humidity = hum;
  HeatIndex = dht.computeHeatIndex(temp, hum, false);

  if(temperature == "nan" && humidity == "nan" && HeatIndex =="nan"){
  Serial.println("Error reading data from sensor");
 }
      StaticJsonDocument<520> doc;

      doc["DeviceID"] = Mac;
      doc["DeviceName"] = device_name;
      doc["Temperature"] = temperature;
      doc["Humidity"] = humidity;
      doc["HeatIndex"] = HeatIndex;
      char output[520];
      serializeJson(doc, output);
      client.publish("homeAutomation",output);

}

void loop() {
  
   if (!client.connected()) {
    reconnect();
  }
  client.loop();
    long now = millis();
    if (now - lastSent > 2000) {
      lastSent = now;
      TempHum();
  }
  Reset();
}

