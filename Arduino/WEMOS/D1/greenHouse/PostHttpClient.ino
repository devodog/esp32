/**
   PostHTTPClient.ino
   On WEMOS LOLIN D1 mini
   https://docs.wemos.cc/en/latest/d1/d1_mini_3.1.0.html
   ??? What WEMOS is, is still a big question...
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "DHT.h"

#define STASSID "ASUS"
#define STAPSK "Framogtilbakeerlikelangt"

#define HEATER_PIN 1
#define DHTPIN 2     // Digital pin connected to the DHT sensor

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22 (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define MINIMUM_TEMP 14
#define HYSTERESE 4

// Connect device pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect device pin 1
// to 3.3V instead of 5V!
// Connect device pin 2 of the sensor to whatever your DHTPIN is
// Connect device pin 3 (on the right) of the sensor to GROUND (if your sensor has 3 pins)
// Connect device pin 4 (on the right) of the sensor to GROUND and leave the pin 3 EMPTY (if your sensor has 4 pins)
// Connect a 10K resistor from device pin 2 (data) to device pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  pinMode(HEATER_PIN, OUTPUT);

  dht.begin();
  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
   // wait for WiFi connection
   if ((WiFi.status() == WL_CONNECTED)) {
      WiFiClient client;
      HTTPClient http;

      char postPayload[256];
      char sensorData[] = "{\"SensorID\":\"0001\",\"NodeName\":\"miniDrivhus\",\"Temperature\":";
      char heaterState[4] = {0};
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      float h = dht.readHumidity();
      float t = dht.readTemperature(); // Read temperature as Celsius (the default)

      if (t < MINIMUM_TEMP) {
         digitalWrite(HEATER_PIN, HIGH);
         heaterState = "ON";
      }
      else if (t > (MINIMUM_TEMP + HYSTERESE)) {
         digitalWrite(HEATER_PIN, LOW);
         heaterState = "OFF";
      }


      if ((t-(int)t) < 0.1)
         snprintf(postPayload, sizeof(postPayload), "%s%d.0,\"Humidity\":%2.1f,\"Heater\":%s}", sensorData, (int)t, h, heaterState);
      else
         snprintf(postPayload, sizeof(postPayload), "%s%2.1f,\"Humidity\":%2.1f,\"Heater\":%s}", sensorData, t, h, heaterState);    

      Serial.printf("json: %s\n", postPayload);

      Serial.print("[HTTP] begin...\n");
      // configure traged server and url
      http.begin(client, "http://midtskips.no/dht/receive_post.php");  // HTTP
      http.addHeader("Content-Type", "application/json");

      Serial.print("[HTTP] POST...\n");
      
      // start connection and send HTTP header and body
      int httpCode = http.POST(postPayload);
      // httpCode will be negative on error
      if (httpCode > 0) {
         // HTTP header has been send and Server response header has been handled
         Serial.printf("[HTTP] POST... code: %d\n", httpCode);

         // file found at server
         if (httpCode == HTTP_CODE_OK) {
            const String& payload = http.getString();
            Serial.println("received payload:\n<<");
            Serial.println(payload);
            Serial.println(">>");
         }
      } 
      else {
         Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
   }
   else {
      // WiFi connection is broken - we'll try to reestablish it
      WiFi.begin(STASSID, STAPSK);
      int reconAttempts = 0; 
      while (WiFi.status() != WL_CONNECTED) {
         delay(1000);
         Serial.print(".");
         if (++reconAttempts > 60) { // we'll only wait for one minute to reconnect - If unable to reconnect, we'll try again 
            Serial.println("");
            Serial.printin("Unable to reconnect to WiFi. Will retry later.");
            break;
         }
      }
      Serial.println("");
      Serial.print("Connected! IP address: ");
      Serial.println(WiFi.localIP());   
   }

   for (int i = 0; i < 15; i++) { // The loop will repeat every 15 minutes...
      delay(60000);
   }
}
