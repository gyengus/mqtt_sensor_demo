#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include "vcc.h"
#include "Timer.h"
#include "config.h"
#include "PubSubClient.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"

extern "C" {
  ADC_MODE(ADC_VCC);
}

#define STATLED D0

#define VCC_TRESHOLD 3000

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_BME280 bme280;
Timer t;

String vcc = "0";
float temperature = 0.0;
int pressure = 0;
float humidity = 0.0;
int ledstate = 0;

void checkVcc() {
  int vccmv = getVcc();
  vcc = String(vccmv / 1000) + "." + String(vccmv % 1000);
}

void getTemperature() {
  temperature = bme280.readTemperature();
}

void getPressure() {
  pressure = bme280.readPressure();
}

void getHumidity() {
  humidity = bme280.readHumidity();
}

void measure() {
  checkVcc();
  getTemperature();
  getPressure();
  getHumidity();
}

void handleRoot() {
  measure();
  String body = "<!doctype html><html lang='hu'><head><title>Hőmérő</title><meta charset='UTF-8'/><style>body{margin: 0px;padding: 20px;font: 14px Verdana,Arial,sans-serif;}#container{display: block;border: 1px solid #dddddd;border-radius: 3px;box-shadow: 0 3px 5px 0 rgba(50,50,50,.25);width: 200px;max-width: 500px;margin: 0px auto;background: #ffffff;padding: 10px;}#sensor_name{font-size: 1.2em;text-align: center;font-weight: bold;margin-bottom: 10px;border-bottom: 1px solid #dddddd;}.left{float: left;width: 120px;}.right{float: right;text-align: right;width: 80px;}.clearboth{clear: both;height: 5px;}</style></head><body><div id='container'><div id='sensor_name'>" + String(SENSOR_NAME) + "</div>"
              + "<div class='left'>Hőmérséklet</div><div class='right'>" + String(temperature) + " °C</div>"
              + "<div class='clearboth'></div>"
              + "<div class='left'>Légnyomás</div><div class='right'>" + pressure + " Pa</div>"
              + "<div class='clearboth'></div>"
              + "<div class='left'>Páratartalom</div><div class='right'>" + humidity + " %</div>"
              + "<div class='clearboth'></div>"
              + "<div class='left'>Tápfeszültség</div><div class='right'>" + vcc + " V</div>"
              + "<div class='clearboth'></div>"
              + "<div class='left'>LED</div><div class='right'>" + ledstate + "</div>"
              + "<div class='clearboth'></div>"
              + "</div></body></html>";
  server.send(200, "text/html", body);
}

void handleJSON() {
  measure();
  String body = "{\"sensor_name\": \"" + String(SENSOR_NAME) + "\", \"temperature\": " + String(temperature) + ", \"pressure\": " + pressure + ", \"humidity\": " + humidity + ", \"vcc\": " + vcc + ", \"led\": " + ledstate + "}";
  server.send(200, "application/json", body);
}

void connectToMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = SENSOR_NAME;
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("demo/led");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 20 seconds");
      delay(20000);
    }
  }
}

void receiveFromMQTT(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] Size: " + String(length) + ", payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (payload[0] == '1') {
    digitalWrite(STATLED, false);
    ledstate = 1;
  } else {
    digitalWrite(STATLED, true);    
    ledstate = 0;
  }
}

void publishToMQTT() {
  if (client.connected()) {
    measure();

    client.publish("demo/led", String(ledstate).c_str(), true);

    client.publish("demo/temperature", String(temperature).c_str(), true);

    client.publish("demo/pressure", String(pressure).c_str(), true);

    client.publish("demo/humidity", String(humidity).c_str(), true);

    client.publish("demo/vcc", vcc.c_str(), true);
  }
}

void setup() {
  ESP.wdtEnable(1000);
  pinMode(STATLED, OUTPUT);
  digitalWrite(STATLED, true);

  Serial.begin(115200);
  delay(10);

  Serial.println();

  Wire.begin(D3, D4);
  Wire.setClock(100000);
  if (!bme280.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1) {
      ESP.wdtFeed();
      delay(200);
    }
  }

  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to WiFi network ");
  Serial.println(sta_ssid);
  WiFi.begin(sta_ssid, sta_password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    i++;
    if (i >= 20) break;
  }
  Serial.println("");
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP address: ");  
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("Unable to connect");
  }

  Serial.println("");

  server.on("/", handleRoot);
  server.on("/json", handleJSON);
  server.begin();

  client.setServer(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT);
  client.setCallback(receiveFromMQTT);
  t.every(MEASURE_INTERVALL, publishToMQTT);
}

void loop() {
  ESP.wdtFeed();
  server.handleClient();
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();
  t.update();
}

