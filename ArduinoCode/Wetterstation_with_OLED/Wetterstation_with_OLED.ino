#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "fonts.h"

#include <ESP8266WiFi.h>
#include "DHT.h"

#define DHTPIN 14     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE); // 30 is for cpu clock of ESP8266 80Mhz
SSD1306  display(0x3c, 12, 13);

const char* ssid     = "ESP8266AP"; //Auenland 2,4GHz
const char* password = "";//61211591054673063607

float h;
float t;

unsigned long ulReqcount;
unsigned long ulReconncount;
// Create an instance of the server on Port 80
WiFiServer server(80);

/*----------------------------------
 * ------------SETUP----------------
 -----------------------------------*/

void setup() {
  // setup globals
  ulReqcount=0; 
  
  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);

  //initialize AM2302
  dht.begin();
  delay(10);
  
  // start serial
  Serial.begin(9600);
  delay(1);
  
  // AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  server.begin();

  // Reading temperature or humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        float h = dht.readHumidity();
        // Read temperature as Celsius
        float t = dht.readTemperature();
        // Read temperature as Fahrenheit
        //float t = dht.readTemperature(true);

  //initialise OLED

  display.init();
  display.flipScreenVertically();

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(Roboto_Condensed_Bold_Bold_16);
  display.drawString(64, 32, "TEST");
  display.display();
  
}




/*----------------------------------
 * -------------MAIN----------------
 -----------------------------------*/

void loop() {

  // Reading temperature or humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        h = dht.readHumidity();
        // Read temperature as Celsius
        t = dht.readTemperature();
        // Read temperature as Fahrenheit
        //float t = dht.readTemperature(true);
/*
        Serial.print("Humidity=");
        Serial.println(h);

        Serial.print("Theperature=");
        Serial.println(t);
        delay(1000);*/

        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_10);
        display.drawString(64, 1, "Temperature/Humidity");
        display.setFont(ArialMT_Plain_24);
        display.drawString(66,  13, String(t) + "°C");
        display.drawString(66,  40, String(h) + "%");
        display.display();
 
/*----------------------------------
 * ----------Web Server-------------
 -----------------------------------*/

// Check if a client has connected
  
  WiFiClient client = server.available();
  if (!client) 
  {
    
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
    Serial.println("client connection time-out!");
    return; 
  }
  
  // Read the first line of the request
  String sRequest = client.readStringUntil('\r');
  Serial.println(sRequest);
  client.flush();
  
  // stop client, if request is empty
  if(sRequest=="")
  {
    Serial.println("empty request! - stopping client");
    client.stop();
    return;
  }
  
  // get path; end of path is either space or ?
  // Syntax is e.g. GET /?pin=MOTOR1STOP HTTP/1.1
  String sPath="",sParam="", sCmd="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0)
  {
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
    
    // are there parameters?
    if(iEndSpace>0)
    {
      if(iEndQuest>0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart,iEndSpace);
      }
    }
  }
  
  ///////////////////////////////////////////////////////////////////////////////
  // output parameters to serial, you may connect e.g. an Arduino and react on it
  ///////////////////////////////////////////////////////////////////////////////
  if(sParam.length()>0)
  {
    int iEqu=sParam.indexOf("=");
    if(iEqu>=0)
    {
      sCmd = sParam.substring(iEqu+1,sParam.length());
      Serial.println(sCmd);
    }
  }
  
  
  ///////////////////////////
  // format the html response
  ///////////////////////////
  String sResponse,sHeader;
  
  ////////////////////////////
  // 404 for non-matching path
  ////////////////////////////
  if(sPath!="/")
  {
    sResponse="<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>";
    
    sHeader  = "HTTP/1.1 404 Not found\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
  ///////////////////////
  // format the html page
  ///////////////////////
  else
  {
    ulReqcount++;
    sResponse  = "<html><head><title>Portable weatherstation</title><meta http-equiv=\"refresh\" content=\"5\" /></head><body>";
    sResponse += "<font color=\"#000000\"><body bgcolor=\"#FFFFFF\">";
    sResponse += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">";
    sResponse += "<h1>Portable weatherstation</h1>";
    sResponse += "here you can see the data provided by the Sensors.<BR>";
    sResponse += "<BR>AM2302:";
    sResponse += "<BR>Themperature= ";
    sResponse += t;
    sResponse += "<BR>Humidity= ";
    sResponse += h;
    sResponse += "<FONT SIZE=+1>";
    sResponse += "<p><a href=\"?pin=REFRESH\"><button>refresh</button></a>&nbsp;";
    
    
    //////////////////////
    // react on parameters
    //////////////////////
    if (sCmd.length()>0)
    {
      
      // switch GPIO
      if(sCmd.indexOf("REFRESH")>=0)
      {
         /*
          * CODE
          */
      }
    }
    
    sResponse += "<FONT SIZE=-2>";
    sResponse += "<BR>Aufrufz&auml;hler="; 
    sResponse += ulReqcount;
    sResponse += " - Verbindungsz&auml;hler="; 
    sResponse += ulReconncount;
    sResponse += "</body></html>";
    
    sHeader  = "HTTP/1.1 200 OK\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
  
  // Send the response to the client
  client.print(sHeader);
  client.print(sResponse);
  
  // and stop the client
  client.stop();
  Serial.println("Client disonnected");


 
}

/*----------------------------------
 * ----------FUNKTIONS--------------
 -----------------------------------*/





