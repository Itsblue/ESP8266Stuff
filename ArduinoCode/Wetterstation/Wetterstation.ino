#include <ESP8266WiFi.h>
#include "DHT.h"

#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE, 30); // 30 is for cpu clock of ESP8266 80Mhz

void setup() {
  Serial.begin(9600);
  dht.begin();
  delay(10);
}

void loop() {

/*
 *Temperature and Humidity AM2302
 */
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
   // Read temperature as Fahrenheit
  //float t = dht.readTemperature(true);

  
/*
 * Te
 */
}



