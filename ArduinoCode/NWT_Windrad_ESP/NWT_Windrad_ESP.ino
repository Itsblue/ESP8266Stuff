#include <ESP8266WiFi.h>
#include "icons.h"
#include "images.h"
#include "fonts.h"
#include "SSD1306.h"



const int reed = 13;           // reed-contact is attached to GPIO13.
const char* ssid     = "YOUR SSID HERE";
const char* password = "YOUR PASSWORD HERE";

int progress = 0;
int wifitimeout = 0;

float wheel_diameter = 125;      //outer wheel diameter (in cm)
float start_time = 0;          //variable nesseceary for timer
float stop_time = 0;           //variable nesseceary for timer
float stopped_time = 0;        //time needed for one cycle
float wind_speed = 0;              //varible to store Wind speed
float wind = 0;

SSD1306  display(0x3c, 12, 14);   //initialize display for sda and scl GPOI`s 12 and 14 at the adress 0x3c

/*----------------------------------
 * ------------SETUP----------------
 -----------------------------------*/

void setup() {
  // put your setup code here, to run once:


  pinMode(reed, INPUT);
 
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);
  Serial.begin(9600);

  display.init();

  display.flipScreenVertically();

   
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
      
 WiFi.begin(ssid, password);

 while (WiFi.status() != WL_CONNECTED && wifitimeout < 5) {
   

  delay(50);
  Serial.print(".");
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(Roboto_Condensed_Bold_Bold_16);
  display.drawString(64, 8, "Connecting to WiFi");
  display.drawProgressBar(10, 28, 108, 12, progress);
  display.drawString(64, 48, String(ssid));
  display.display();
  progress++;
  
  if(progress > 100){progress = 0;wifitimeout++;}
 }
 
if(wifitimeout < 5){
    display.clear();
  display.drawXbm(32, 0, 50,50, tick_bits);
  display.drawString(64, 50, "connected!");
  display.display();
  delay(2000);
}
else{
    display.clear();
  display.drawXbm(32, 0, 50,50, error_bits);
  display.drawString(64, 50, "timeout!");
  display.display();
  delay(10000);
}
}

/*----------------------------------
 * -------------MAIN----------------
 -----------------------------------*/

void loop() {
 
  int i=0;
  while(i<1000){
  if(!digitalRead(reed)){
    digitalWrite(2, 1);
    Serial.println("HIGH");

    display.clear();
      display.setTextAlignment(TEXT_ALIGN_CENTER);

      display.setFont(Calligraffitti_20);
      display.drawString(64,  20, F("HIGH"));
      display.display();
      
  }
  else{
    digitalWrite(2, 0);
    Serial.println("LOW");
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    //display.setFont(Roboto_Condensed_Bold_Bold_16);
    display.setFont(Calligraffitti_20);
    display.drawString(64,  20, F("LOW"));
    display.display();
  }
  //i++;
  delay (50);
  //display.drawString(0,  0, String(i));
  }
}
  
  /*
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    //display.setFont(Roboto_Condensed_Bold_Bold_16);
    display.setFont(Calligraffitti_20);
    //display.drawString(64,  20, String(TestWindSpeed()));



    wind = TestWindSpeed();
    
    Serial.println(wind);
    display.drawString(64,  20, String(wind));
    display.display();
    
    //i++;
    delay(50);
 
}*/

/*----------------------------------
 * ----------FUNKTIONS--------------
 -----------------------------------*/
/*
int TestWindSpeed(){

  wheel_diameter = wheel_diameter / 100;
  
  while (!digitalRead(reed)){delay(10);}   //wait for the first signal from the reed contact.
  
  start_time = millis();              // save start time
  
   while (!digitalRead(reed)){delay(10);}  //wait for another signal from the reed contact.

  stop_time = millis();               //save stop time
  stopped_time = stop_time - start_time;   //calculate needed time

   wind_speed = 1 / (stopped_time * wheel_diameter);    //calculate wind speed
  
}
*/
