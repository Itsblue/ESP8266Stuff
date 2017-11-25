
#include <vector>
#include "SSD1306.h"                          // this is the OLED display library
#include "fonts.h"                            // fonts library to support the display output

#include <ESP8266WiFi.h>                      // nodemcu wifi module support 
#include "DHT.h"                              // DHT22 temperature and humidity sensor library
#include "Wetterstation_with_OLED.h"

// function declaration ...
void readTempHum( int , /*out*/ float*, /*out*/ float*, /*out*/ float*);
void writeTempHum(/*in*/ float , /*in*/ float , /*in*/ float ,/*in*/ String);
void sendHTMLResponse( /*in*/ String path,/*in*/ float [],/*in*/ float [],/*in*/ const String [], /*in*/ WiFiClient client);
void getClientRequest( /*out*/ bool* active, /*out*/ String* cmd, /*out*/ String* path, /*out*/ WiFiClient* client);
void execCmds ( /*in*/ String sCmd );

// internal defines for the temperature/humidity sensors ... do not change if hardware is the same ...
#define DHTTYPE DHT22                         // DHT22 is our temperatur and humidity sensor (AM2302)
#define MIN_WAIT_TIME_MS 2000                 // the minimum of time in ms to wait before reading the sensors

float humidity[sizeof(IN_DHTPIN)];              // array of measured humidity
float temperature[sizeof(IN_DHTPIN)];           // array of measured temperature
float heat[sizeof(IN_DHTPIN)];                  // array of measured heats

// internal defines for the OLED display ...
#define DISPSDA 5                            // display pin1 - was 10
#define DISPSCI 4                             // display pin2 - was 9
#define DISPADDR 0x3c                         // display address
SSD1306  display(DISPADDR, DISPSDA, DISPSCI); //initialize display for sda and scl GPOI`s 12 and 14 at the adress 0x3c
#define MAX_CONTRAST 128
#define MIN_CONTRAST 0

// define the diagramm axes 
#define X_AXES_START 10
#define X_AXES_LENGTH 90
#define Y_AXES_START 7
#define Y_AXES_LENGTH 50

// internal defines for the serial monitor ...
#define SERIAL_BAUDRATE 9600                  // serial monitor baudrate

// Create an instance of the server / wifi module
WiFiServer server(SERVER_PORT);     
WiFiClient client;


// global variables ...
String clientCmd = "";
String clientPath = "";

bool activeRequest = false;

unsigned long ulReqcount = 0;
unsigned long ulReconncount = 0;
unsigned long lastSensorRead = 0;
unsigned long actualTime = MIN_WAIT_TIME_MS;

int active_sensor = 0;
#define nr_sensors (sizeof(IN_DHTPIN)/sizeof(char *)) //array size 
//int nr_sensors = sizeof IN_DHTPIN;
std::vector <DHT> dhtsensor;

// lets do some sta

/*----------------------------------
 * ------------SETUP----------------
 -----------------------------------*/

// code in this section is done once ...

void setup() {
  // start serial monitor
  Serial.begin(SERIAL_BAUDRATE);
  delay(1);

  Serial.println(nr_sensors);

  // setup globals
  ulReqcount=0;
  
  // start DHT22 ... temerature and humidity sensor
  for(int nr=0; nr < nr_sensors; nr++){
    dhtsensor.push_back(DHT(IN_DHTPIN[nr],DHTTYPE));
    dhtsensor[nr].begin();
  }
  
  // start Wifi in AccessPoint mode
  // TODO: start wifi in client mode if at home ...
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  server.begin();
  
  //initialise OLED and display Welcome Message ...
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(Roboto_Condensed_Bold_Bold_16);
  display.clear();
  display.setContrast(MIN_CONTRAST);
  display.drawString(64, 32, "Willkommen!");
  display.display();
  fade_in(MIN_CONTRAST);
  delay(2000);
  fade_out(MAX_CONTRAST);
  display.clear();
  display.setContrast(MAX_CONTRAST);
}

/*----------------------------------
 * -------------MAIN----------------
 -----------------------------------*/
void loop() {

  
  
  draw_axis(0,0,0,0);
  display.display();
  


  
}

void fade_out(char actual_contrast){
  Serial.print("fade out contrast: ");
  for(int contrast = actual_contrast; contrast>=MIN_CONTRAST; contrast--){
    Serial.println(contrast);
    display.setContrast(contrast);
    display.display();
  }
}

void fade_in(char actual_contrast){
  Serial.print("fade in contrast: ");
  for(int contrast = actual_contrast; contrast<=MAX_CONTRAST; contrast++){
    Serial.println(contrast);
    display.setContrast(contrast);
    display.display();
  }
}

void draw_axis(float minxval, float maxxval, float minyval , float maxyval){

  //--- rectangular ... x.y , width , height
  display.drawRect(X_AXES_START, Y_AXES_START, X_AXES_LENGTH, Y_AXES_LENGTH);
  //--- add scales to the x and y axes 
  // Draw a line from position 0 to position 1
  for(int x = 15;x<= 80;x=x+15){
    display.drawLine(x, Y_AXES_START + Y_AXES_LENGTH ,x, Y_AXES_START + Y_AXES_LENGTH - 3);
    display.drawLine(x, Y_AXES_START  ,x, Y_AXES_START + 3);
  }
}

void temp_loop() {
  
  // read the sensors and sent data to display ... 
  if( actualTime -lastSensorRead >= MIN_WAIT_TIME_MS )
  {
    // sensors are read after a specified time in milliseconds ... 
    readTempHum( active_sensor, &temperature[active_sensor], &humidity[active_sensor], &heat[active_sensor]);
    writeTempHum( temperature[active_sensor], humidity[active_sensor], heat[active_sensor], DISPLAY_TITLE[active_sensor] );
    lastSensorRead = millis();
    actualTime = millis();
    if(active_sensor < nr_sensors-1){
      active_sensor++;
    }
    else{
      active_sensor=0;
    }
  }
  else
  {
    actualTime = millis();
  }
  
  // check if there is a client connected to the Webserver ...
  getClientRequest( &activeRequest, &clientCmd, &clientPath, &client );
  if(activeRequest)
  {
    // execute the cmmands and send response to client ...
    execCmds( clientCmd );
    sendHTMLResponse( clientPath, temperature, humidity, DISPLAY_TITLE , client);
  }
}

/*----------------------------------
 * ----------FUNCTIONS-------------
 -----------------------------------*/
void readTempHum( /* in  */ int sensor,
                  /* out */ float* temp,
                  /* out */ float* hum,
                  /* out */ float* heat)
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)

  *hum     = (float)dhtsensor[sensor].readHumidity();      // read the indoor humidity
  *temp    = (float)dhtsensor[sensor].readTemperature();   // read the indoor tempeature in celcius
  *heat    = (float)dhtsensor[sensor].computeHeatIndex(*temp, *hum, false);;   // compute the heat index in celcius
}


void writeTempHum(  /* in */ float temp,
                    /* in */ float hum,
                    /* in */ float heat,
                    /* in */ String title)
{
  // Writing temperature and humidity to the display and serial monitor !
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 0, title);
  display.setFont(ArialMT_Plain_24);
  display.drawString(64,  30, String(heat) + "°C");
  display.setFont(ArialMT_Plain_16);
  display.drawString(28,  14, String(temp) + "°C");
  display.drawString(64,  14, "|");
  display.drawString(100,  14, String(hum) + "%");
  display.setFont(ArialMT_Plain_10);
  display.drawString(64,  54, String("Gefühlte Temperatur"));
  
  display.display();

}

void sendHTMLResponse(  /* in */ String path,
                        /* in */ float temp[],
                        /* in */ float hum[],
                        /* in */ const String title[],
                        /* in */ WiFiClient client )
{
  ///////////////////////////
  // format the html response
  ///////////////////////////
  String sResponse,sHeader;

  ////////////////////////////
  // 404 for non-matching path
  ////////////////////////////
  if(path!="/")
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
    sResponse += "Data provided by the Sensors.<BR>";

    for(int sensor=0;sensor<nr_sensors;sensor++){
      sResponse += title[sensor];
      sResponse += "<BR>Themperature= ";
      sResponse += temp[sensor];
      sResponse += "<BR>Humidity= ";
      sResponse += hum[sensor];
    }
    sResponse += "<FONT SIZE=+1>";
    sResponse += "<p><a href=\"?button=REFRESH\"><button>Read Temperature and Humidity</button></a>&nbsp;";
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

    // Send the response to the client
    client.print(sHeader);
    client.print(sResponse);
    Serial.println("Response sent to client");

    // and stop the client
    client.stop();
    //Serial.println("Client disconnected");

  } //path ends with a /
}


void getClientRequest( /* out */ bool* active,
                       /* out */ String* cmd,
                       /* out */ String* path,
                       /* out */ WiFiClient* client)
{

  /*----------------------------------
   * ----------Web Server-------------
  -----------------------------------*/
  *active = false;
  
  // Check if a client has connected
  WiFiClient tmp_client = server.available();
  if (tmp_client) 
  {
    return;
  }

  *active = true;
  // Wait until the client sends some data
  Serial.println("New client tries to connect ...");
  unsigned long ultimeout = millis()+250;
  while(!tmp_client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
    Serial.println("Client connection time-out!");
    return; 
  }
  
  // Read the first line of the request
  String sRequest = tmp_client.readStringUntil('\r');
  Serial.print("Client request is: ");
  Serial.println(sRequest);
  tmp_client.flush();
  
  // stop client, if request is empty
  if(sRequest=="")
  {
    Serial.println("Client sends empty request! - stopping client connection");
    tmp_client.stop();
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
      Serial.print("Clients sends cmd:");
      Serial.println(sCmd);
    }
  }
  else
  {
    Serial.print("Clients sends no cmd.");
  }
  // send back cmd 
  *cmd = sCmd;
  *client = tmp_client; 
}


void execCmds ( /* in */ String sCmd )
{
      //////////////////////
    // react on parameters
    //////////////////////

    Serial.print("Start executing of cmds:");
    Serial.println(sCmd);
    
    if (sCmd.length()>0)
    {

      // do whatever is requested here ...
      if(sCmd.indexOf("REFRESH")>=0)
      {
         /*
          * CODE to react to the readTempHum button ... nothing to do right now.
          */
      }
    }
}



/*----------------------------------
 * ----------FUNCTIONS--------------
 -----------------------------------*/





