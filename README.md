# MQTT sensor demo

It's a sample te demonstrate how to send sensor datas from Arduino to MQTT broker and controll LED over MQTT topic.

You must set some variables in config.h:
```c
#define SENSOR_NAME "sensor_name"
#define MQTT_BROKER_ADDRESS "ip or hostname"
#define MQTT_BROKER_PORT 1883
const char *sta_ssid = "your wifi ssid";
const char *sta_password = "your wifi password";
```
