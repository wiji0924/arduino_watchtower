#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <WiFiUdp.h> 
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define WIFI_SSID "ngaling"  // ssid-wifi name
#define WIFI_PASSWORD "1234567890"  // wif-password

// front ultrasonic sensor
// trig pin = D5
// echo pin = D6

const int FrontTrigPin = 14;
const int FrontEchoPin = 12;

// left ultrasonic sensor
// trig pin = D7w
// echo pin = D0
const int LeftTrigPin = 13;
const int LeftEchoPin = 16;

// right ultrasonic sensor
// trig pin = D4
// echo pin = D8
const int RightTrigPin = 2;
const int RightEchoPin = 15;

// defines variables
long Frontduration;
long Leftduration;
long Rightduration;

float waterLevel = 0.0;
float prevwaterLevel =0.0;
float waterLevelfront = 0.0;
float waterLevelright = 0.0;
float waterLevelleft = 0.0;

unsigned long lastTime = 0;
unsigned long timerDelay = 3000;

const char* serverName = "http://13.239.116.14:8090/history";

// Create AsyncWebServer object on port 8088
AsyncWebServer server(8088);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<body>
    <h1>ok</h1>
</body>
</html>)rawliteral";

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {
  Serial.begin(9600);
    // connect to wifi.   
  Serial.print("Test");  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  
  Serial.print("connecting");  
  while (WiFi.status() != WL_CONNECTED) {  
   Serial.print(".");  
    delay(500);  
  }  
  Serial.println();  
  Serial.print("connected: ");  
 Serial.println(WiFi.localIP());  

  pinMode(FrontTrigPin, OUTPUT); 
  pinMode(FrontEchoPin, INPUT); 
  pinMode(LeftTrigPin, OUTPUT); 
  pinMode(LeftEchoPin, INPUT); 
  pinMode(RightTrigPin, OUTPUT); 
  pinMode(RightEchoPin, INPUT); 

  server.on("/check", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/waterlvl", HTTP_GET, [](AsyncWebServerRequest *request) {
    String waterLVL = String(waterLevel);
    request->send(200, "application/json", "{\"water_level\": \"" + waterLVL + "\"}");
  });
  
  server.begin();
}

void loop() {

    if ((millis() - lastTime) > timerDelay) {
    GetWaterLevel();
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);
      
      // If you need an HTTP request with a content type: application/json, use the following:
      http.addHeader("Content-Type", "application/json");
      // JSON data to send with HTTP POST
      String httpRequestData = "{\"WaterLevel\":\"" + String(waterLevel) + "\"}";           
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
     
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
    Serial.println("padua egup");
  }
}

void GetWaterLevel(){

  // FRONT ULTRASONIC SENSOR
  digitalWrite(FrontTrigPin, LOW); 
  delayMicroseconds(2);

  digitalWrite(FrontTrigPin, HIGH);
  delayMicroseconds(20);

  digitalWrite(FrontTrigPin, LOW);

  Frontduration = pulseIn(FrontEchoPin, HIGH);
  //Serial.print(Frontduration);
  float frontdata = Frontduration * 0.034 / 2; 
  float distanceInMetersF = (frontdata/100);

  float distanceMappedF = floatMap(distanceInMetersF, 0.2, 1.2, 0.0, 5.0);
  waterLevelfront = 18.5 - distanceMappedF;


  // LEFT ULTRASONIC SENSOR
  digitalWrite(LeftTrigPin, LOW); 
  delayMicroseconds(2);

  digitalWrite(LeftTrigPin, HIGH);
  delayMicroseconds(20);

  digitalWrite(LeftTrigPin, LOW);

  Leftduration = pulseIn(LeftEchoPin, HIGH);

  float leftdata   = Leftduration * 0.034 / 2; 
  float distanceInMetersL = (leftdata/100);

  float distanceMappedL = floatMap(distanceInMetersL, 0.2, 1.2, 0.0, 5.0);
  waterLevelleft = 18.5 - distanceMappedL;

  // RIGHT ULTRASONIC SENSOR
  digitalWrite(RightTrigPin, LOW); 
  delayMicroseconds(2);

  digitalWrite(RightTrigPin, HIGH);
  delayMicroseconds(20);

  digitalWrite(RightTrigPin, LOW);

  Rightduration = pulseIn(RightEchoPin, HIGH);

  float rightdata    = Rightduration * 0.034 / 2; 
  float distanceInMetersR = (rightdata /100);

  float distanceMappedR = floatMap(distanceInMetersR, 0.2, 1.2, 0.0, 5.0);
  waterLevelright = 18.5 - distanceMappedR;

  Serial.println(waterLevelleft);
  Serial.println(waterLevelfront);
  Serial.println(waterLevelright);

  float tolerance = 0.5; // Adjust the tolerance value as needed

  // Check if the three sensor values are close to each other
  if (abs(waterLevelfront - waterLevelleft) <= tolerance &&
      abs(waterLevelfront - waterLevelright) <= tolerance &&
      abs(waterLevelleft - waterLevelright) <= tolerance) {
    // All three sensor values are close to each other

    // Calculate the average of the three sensor values
    waterLevel = (waterLevelfront + waterLevelleft + waterLevelright) / 3.0;
  } else {
    // At least one sensor value is not close to the others

    // Check which two sensors are closer to each other
    if (abs(waterLevelfront - waterLevelleft) <= tolerance &&
        abs(waterLevelfront - waterLevelright) <= tolerance) {
      // Front and Left sensors are close, average them
      waterLevel = (waterLevelfront + waterLevelleft) / 2.0;
    } else if (abs(waterLevelleft - waterLevelright) <= tolerance) {
      // Left and Right sensors are close, average them
      waterLevel = (waterLevelleft + waterLevelright) / 2.0;
    } else if (abs(waterLevelfront - waterLevelright) <= tolerance) {
      // Front and Right sensors are close, average them
      waterLevel = (waterLevelfront + waterLevelright) / 2.0;
    } else {
      // None of the sensors are close to each other, choose one arbitrarily
      waterLevel = prevwaterLevel;
    }
  }

  // Set prevwaterLevel to the current waterLevel
  prevwaterLevel = waterLevel;

  delay(100);
}



