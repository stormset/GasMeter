#include <Ticker.h>
#include <Schedule.h>
#include <Arduino.h>
#include <WiFiManager.h> 
#define WEBSERVER_H
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ESPDash.h>
#include <Filters.h>

/* WIFI ACCESS PORTAL */
const char* AP_name = "Gas Meter"; // when connection fails, the opened access point name
bool portalRunning = false; // global state variable


/* PARAMETERS - should be configurable through the UI */
uint32_t SAMPLING_PERIOD = 200;
float SENSOR_THRESHOLD = 3.0f;
float NOISE_THRESHOLD = 0.3f;
const uint32_t SENSITIVITY = 60; // in percentage [1..100]
const uint32_t LITERS_PER_PULSE = 10;
const uint32_t Qmax = 6; // m3/hr maximal m3 output in 1 hour length interval
const uint32_t CURRENT_LITERS = 9079000;
const float m3Price = 104.0;

/* PINOUT */
const uint8_t SENSOR_PIN = A0;

Card *uiAccumulatedCubicMeters;
Card *uiCurrentLiters; // since turn on
Card *uiCurrentPrice; // since turn on

/* Timer */
Ticker timer;

/* Start Webserver */
AsyncWebServer server(80);

/* Wifi connection manager */
WiFiManager wifiManager;

/* Attach ESP-DASH to AsyncWebServer */
ESPDash dashboard(&server); 

//FilterTwoPole filter(1, 1, 0); // (float frequency0, float qualityFactor, float xInit)
FilterOnePole filter( INTEGRATOR, 1.0, 0 );

/* GLOBALS - TO BE REMOVED */
unsigned long time_now = 0;
unsigned long last_pulse = 0;

uint32_t accumulatedLiters = CURRENT_LITERS;
uint32_t sinceTurnOnLiters = 0;

// function pre-declarations
void readSensor();
void onPulseDetected(int);
void doWiFiManager();
void changeSensitivity(int);

void setup() {
   Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  /* Initialize the UI elements */
  // temperature and humidity
  uiAccumulatedCubicMeters = new Card(&dashboard, GENERIC_CARD, "Összes fogyasztás (m3)");
  uiCurrentLiters =  new Card(&dashboard, GENERIC_CARD, "Bekapcsolás óta (m3)");
  uiCurrentPrice =  new Card(&dashboard, GENERIC_CARD, "Bekapcsolás óta (Ft)");

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.autoConnect(AP_name);

  /* OTA */
  AsyncElegantOTA.begin(&server);
  
  /* Start AsyncWebServer */
  server.begin();

  /* fine-tuning parameters - for testing purposes, while in development */
  server.on("/change", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("all")) {
        accumulatedLiters = (request->getParam("all")->value()).toInt();
    }
    if (request->hasParam("current")) {
        sinceTurnOnLiters = (request->getParam("current")->value()).toInt();
    }
    if (request->hasParam("sensitivity")) {
        changeSensitivity((request->getParam("sensitivity")->value()).toInt());
    }
    if (request->hasParam("threshold")) {
        SENSOR_THRESHOLD = (request->getParam("threshold")->value()).toInt();
    }
    if (request->hasParam("noise")) {
        SENSOR_THRESHOLD = (request->getParam("threshold")->value()).toFloat();
    }
    if (request->hasParam("period")) {
        SAMPLING_PERIOD = (request->getParam("noise")->value()).toInt();
    }
    
    // update UI
    onPulseDetected(0);
    
    request->send(200, "text/plain", "OK");
  });

  server.on("/zero", HTTP_GET, [] (AsyncWebServerRequest *request) {
    sinceTurnOnLiters = 0;
    request->send(200, "text/plain", "OK");
  });
  //////////////////////////////////////////////////////////////////////

  changeSensitivity(SENSITIVITY);

  // update UI
  onPulseDetected(0);
}

void loop() {
   if((unsigned long)(millis() - time_now) > SAMPLING_PERIOD){
     time_now = millis();
     readSensor();
  }
  wifiManager.process();
}

void readSensor() {
  static bool thresholdHit = false;
  
  filter.input(analogRead(SENSOR_PIN));
  int sensorValue = filter.output();
   Serial.println(sensorValue);
  
  if(sensorValue >= SENSOR_THRESHOLD && (unsigned long)(millis() - last_pulse) > (Qmax - 1) * 1000){
    thresholdHit = true;
    last_pulse = millis();
  }
  else if(thresholdHit == true && sensorValue < NOISE_THRESHOLD){
      thresholdHit = false;
      onPulseDetected(LITERS_PER_PULSE);
   }
}

void onPulseDetected(int consumedAmount){
  accumulatedLiters += consumedAmount;
  sinceTurnOnLiters += consumedAmount;
  uiAccumulatedCubicMeters->update(String((double)accumulatedLiters / 1000));
  uiCurrentLiters->update(String((double)sinceTurnOnLiters / 1000));
  uiCurrentPrice->update((int)round(((double)sinceTurnOnLiters / 1000) * m3Price));
  dashboard.sendUpdates();
}

void doWiFiManager(){
  // is auto timeout portal running
  if(portalRunning) {
    if (WiFi.status() == WL_CONNECTED){
      wifiManager.stopConfigPortal();
      portalRunning = false;
      return;
    }
    
    wifiManager.process(); // do processing
  }
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void changeSensitivity(int newValue){ // [1..100]
  filter.setFrequency(mapfloat(newValue, 1, 100, 10, 1));
}
