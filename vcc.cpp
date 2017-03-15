#include <ESP8266WiFi.h>
#include "vcc.h"

ADC_MODE(ADC_VCC);

int getVcc() {
  return ESP.getVcc();
}

