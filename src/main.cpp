#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include "DHT.h"
#include <Adafruit_Sensor.h>
#include <WiFiManager.h>
#include <Wire.h> 

#include "config.h"

#define DHTPIN 1 // config Dht pin
#define WifiLED 13 // config Wifi pin
#define MqttLED 12 // config Mqtt pin
#define LDRPIN A0 // config LDR pin

#define DHTTYPE DHT22 // DHT 11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
int value = 0;
void callback(char *topic, byte *message, unsigned int length);
void setup_wifi();
WiFiManager wm;
bool res;
  
void setup()
{                  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  pinMode(WifiLED, OUTPUT);
  pinMode(MqttLED, OUTPUT);
  dht.begin();
}

void setup_wifi()
{
  res = wm.autoConnect(ssid, password);
  if (!res)
  {
    Serial.println("Failed to connect");
    wm.resetSettings();
  }
  else
  {

    Serial.println("connected");
    Serial.println(WiFi.SSID());
    digitalWrite(WifiLED, HIGH);
  }
}


void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Feel free to add more if statements to control more GPIOs with MQTT
  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
}

// Convert mac to String
String macToStr(const uint8_t *mac)
{
  String result;
  for (int i = 0; i < 6; ++i)
  {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientName; 
    clientName += "esp32-s";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientName += macToStr(mac);
    clientName += "-";
    clientName += String(micros() & 0xff, 16);
    Serial.print("Connecting to ");
    Serial.print(mqtt_server);
    Serial.print(" as ");
    Serial.println(clientName);
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    if (client.connect((char *)clientName.c_str()),mqttUser,mqttPass)
    {
      // if (client.connect((char*) clientName.c_str()), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      digitalWrite(MqttLED, HIGH);   
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(MqttLED, LOW);

      // Wait 5 seconds before retrying
       //resetwifi();
      delay(5000);
    }
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 10000)
  {
    lastMsg = now;
     float h = dht.readHumidity();
      
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();

      // Read Light sensor
      int l = analogRead(LDRPIN);
      int ldr = map(l, 0, 4095, 100, 0); // for esp32
  //  int light = map(l, 0, 1024, 100, 0); // for esp8266
      
  
      if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
      }

   // compute heat index
     // float hic = dht.computeHeatIndex(t, h, false);

      Serial.println();
      Serial.println("---------------------------");
      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.println(" *C ");
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println(" %");
      Serial.print("Light: ");
      Serial.print(ldr);
      Serial.println(" %");
      //Serial.print("Heat index: ");
      // Serial.print(hic);
      // Serial.println(" *C ");
      Serial.println("---------------------------");

    // Pepare String to send data as json
      String temp = String(t).c_str();
      String humi = String(h).c_str();
      String light = String(ldr).c_str();
   //   String d = String(hic).c_str();

   //   String data_out = ""id":" 2 ","temperature":"+ a + ","humidity":" + b + ","light":" + c + "";
   //   data_out = "{\"humidity\":" + Humout + ", \"temperature\":" + Temout + "}";
      String pub_ldr =  "{\"light\":" + light + "}";
      String pub_dht =  "{\"humidity\":" + humi + ", \"temperature\":" + temp + "}";
      String pub_data = "{\"id\":\"2\", \"humidity\":" + humi + ", \"temperature\":" + temp + ", \"light\":" + light +"}"; 

      char msg_ldr[50];
      char msg_dht[50];
      char msg_data[50];

      pub_ldr.toCharArray(msg_ldr, (pub_ldr.length() + 1));
      pub_dht.toCharArray(msg_dht, (pub_dht.length() + 1));
      pub_data.toCharArray(msg_data, (pub_data.length() + 1));

      if (client.publish(PUB_Topic_Ldr, msg_ldr) && client.publish(PUB_Topic_Dht, msg_dht) && client.publish(PUB_Topic_App, msg_data)) {
        digitalWrite(MqttLED, HIGH);
        delay(500);
        digitalWrite(MqttLED, LOW);
        delay(500);
      }
  }
}