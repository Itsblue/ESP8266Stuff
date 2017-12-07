#include <TimeLib.h>
#include <DS1307RTC.h>
#include <vector>
#include "SSD1306.h"                          // this is the OLED display library
#ifndef FONT_LIB_V3
#include "fonts.h"                            // fonts library to support the display output
#endif


#include "images.h"
#include <ESP8266WiFi.h>                      // nodemcu wifi module support 
#include "DHT.h"                              // DHT22 temperature and humidity sensor library
#include "CarDesk_with_OLED.h"
#include "pitch.h"
#include <JN_Graph.h>
#include "Time.h"

#define membersof(x) (sizeof(x) / sizeof(x[0]))

// internal defines for the temperature/humidity sensors ... do not change if hardware is the same ...
#define DHTTYPE DHT22                         // DHT22 is our temperatur and humidity sensor (AM2302)
#define MIN_WAIT_TIME_MS 1000                 // the minimum of time in ms to wait before reading the sensors

// number of maximum temperatur values that will be stored
#define STORED_VALS 108
#define STORED_ALL_INFO STORED_FIRST+STORED_VALS
#define STORE_INTERVAL_IN_MS 300*MIN_WAIT_TIME_MS     //number of milliseconds OR wait cycles to wait before storing the measured values

// array to do an auto scale of the x axes in the diagram ... define are the scaler, max value and the unit ... 
String x_auto_legend[][3] = {{String(1000.0),"99.0","[s]"},{String(60.0*1000.0),"99.0","[min]"},{String(60.0*60.0*1000.0),"72.0","[h]"},{String(24.0*60.0*60.0*1000.0),"72.0","[d]"}};

#define ONLY_ACTUAL_TEMP 0
#define ALL_TEMPS 1

float humidity[sizeof(IN_DHTPIN)][STORED_ALL_INFO];               // array of measured humidity
float temperature[sizeof(IN_DHTPIN)][STORED_ALL_INFO ];           // array of measured temperature
float heat[sizeof(IN_DHTPIN)][STORED_ALL_INFO];                   // array of measured heats
wstate weather;

// internal defines for the OLED display ...
#define DISPSDA 5                            // display pin1 - was 10
#define DISPSCI 4                             // display pin2 - was 9
#define DISPADDR 0x3c                         // display address
SSD1306  display(DISPADDR, DISPSDA, DISPSCI); //initialize display for sda and scl GPOI`s 12 and 14 at the adress 0x3c
#define MAX_CONTRAST 128
#define MIN_CONTRAST 0

#define X_AXES_START 17
#define X_AXES_LENGTH 108
#define Y_UPPER_AXES_START 15
#define Y_LOWER_AXES_START 53
#define MEAS_POINT_RADIUS 2

JN_Graph graph(STORED_VALS, STORED_FIRST, X_AXES_START, X_AXES_LENGTH, Y_UPPER_AXES_START, Y_LOWER_AXES_START, MEAS_POINT_RADIUS);

// internal defines for the serial monitor ...
#define SERIAL_BAUDRATE 9600                  // serial monitor baudrate

// Create an instance of the server / wifi module
WiFiServer server(SERVER_PORT);     
WiFiClient client;


// global variables ...
String max_scale_x_val = "++";
String max_scale_unit  = "[-]";

String clientCmd = "";
String clientPath = "";

bool activeRequest = false;

unsigned long ulReqcount = 0;
unsigned long ulReconncount = 0;
unsigned long lastSensorRead = -MIN_WAIT_TIME_MS;
unsigned long lastAllSensRead = -STORE_INTERVAL_IN_MS;
unsigned long actualTime = MIN_WAIT_TIME_MS;

screen_e screen = WELCOME;
screen_e lastscreen;

location_sensor active_sensor = AUSSEN;
#define nr_sensors (sizeof(IN_DHTPIN)/sizeof(char *)) //array size 
std::vector <DHT> dhtsensor;

motion_e pirState = NOMOTION;
unsigned long lastmotion = millis();

// lets do some sta
// function declaration ...
void readTempHum( /* in  */int sensor,/* out */ float temp[][STORED_ALL_INFO],/* out */ float hum[][STORED_ALL_INFO],/* out */ float heat[][STORED_ALL_INFO], int all_temps,unsigned long lastread);
void writeTempHum(/*in*/ float , /*in*/ float , /*in*/ float ,/*in*/ String);
void sendHTMLResponse( /*in*/ String path,/*in*/ float [],/*in*/ float [],/*in*/ const String [], /*in*/ WiFiClient client);
void getClientRequest( /*out*/ bool* active, /*out*/ String* cmd, /*out*/ String* path, /*out*/ WiFiClient* client);
void execCmds ( /*in*/ String sCmd );
void fade_in(char actual_contrast);
void fade_out(char actual_contrast);
void draw_main_screen(float temp[][STORED_ALL_INFO],float hum[][STORED_ALL_INFO],float heat[][STORED_ALL_INFO],const String title[], wstate weather);
void set_weather_state(float temp[][STORED_ALL_INFO],float hum[][STORED_ALL_INFO],float heat[][STORED_ALL_INFO],wstate* weather);
void motion_detection(motion_e* pirState, unsigned long* lastmotion);
/*----------------------------------
 * ------------SETUP----------------
 -----------------------------------*/

// code in this section is done once ...

void setup() {
  // start serial monitor
  Serial.begin(SERIAL_BAUDRATE);
  delay(1);

  // setup x axis scaler
  float max_scale_value = 0;
  /*int maxshownvalues = X_AXES_LENGTH;
  if(X_AXES_LENGTH > STORED_VALS){
    maxshownvalues = STORED_VALS;
  }*/
  for(int nr=0;nr<membersof(x_auto_legend);nr++)
  {
    max_scale_value = (float(STORE_INTERVAL_IN_MS) * float(STORED_VALS) * 1.0) / x_auto_legend[nr][0].toFloat();
    Serial.print("max_scal_value=");
    Serial.println(max_scale_value);
    if(max_scale_value <= x_auto_legend[nr][1].toFloat())
    {
      max_scale_x_val = String(max_scale_value,1);
      max_scale_unit  = x_auto_legend[nr][2];
      break;
    }
  }
  Serial.print("max_scale_unit=");
  Serial.println(max_scale_unit);
  Serial.print("max_scale_x_val=");
  Serial.println(max_scale_x_val);
  
  // setup globals
  ulReqcount=0;
  
  // start DHT22 ... temperature and humidity sensor
  for(int nr=0; nr < nr_sensors; nr++){
    dhtsensor.push_back(DHT(IN_DHTPIN[nr],DHTTYPE));
    dhtsensor[nr].begin();
  }

  //PIR
  pinMode(PIR, INPUT); 
  

  //weather
  weather.dayornight = DAY;
  weather.wetordry = DRY;
  weather.iceornoice = NOICE;
  weather.heat = 0.0;
  weather.humidity = 0.0; 
  weather.temperature = 0.0;
  weather.timesecs = millis();

  // start Wifi in AccessPoint mode
  // TODO: start wifi in client mode if at home ...
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  server.begin();
  
  //initialise OLED and display Welcome Message ...
  display.init();
  //display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(Serif_bolditalic_18);
  display.clear();
  display.setContrast(MIN_CONTRAST);
  display.drawString(64, 23, "Willkommen!");
  display.display();
  playMusic();  
  fade_in(MIN_CONTRAST);
  delay(2000);

  // init the temparature sensor arrays for now ...
  for(int nr=0; nr < nr_sensors; nr++){
    temperature[nr][STORED_ACT_VAL]   = 0;
    temperature[nr][STORED_INTERVAL]  = STORE_INTERVAL_IN_MS;
    temperature[nr][STORED_LAST_TIME] = lastAllSensRead;
    temperature[nr][STORED_ACT_CNT]   = 0;
    temperature[nr][STORED_ACT_SUM]   = 0;
    humidity[nr][STORED_ACT_CNT]      = 0;
    humidity[nr][STORED_INTERVAL]     = STORE_INTERVAL_IN_MS;
    humidity[nr][STORED_LAST_TIME]    = lastAllSensRead;    
    humidity[nr][STORED_ACT_SUM]      = 0;
    heat[nr][STORED_ACT_CNT]          = 0;
    heat[nr][STORED_ACT_SUM]          = 0;
    heat[nr][STORED_INTERVAL]         = STORE_INTERVAL_IN_MS;
    heat[nr][STORED_LAST_TIME]        = lastAllSensRead;        
    temperature[nr][STORED_ALL_SUM]   = 0;
    humidity[nr][STORED_ALL_SUM]      = 0;
    heat[nr][STORED_ALL_SUM]          = 0;    
    readTempHum( nr, temperature, humidity, heat, ONLY_ACTUAL_TEMP, lastAllSensRead);
    for (int tempnr=STORED_FIRST; tempnr < (STORED_VALS + STORED_FIRST-1); tempnr++)
    {
      temperature[nr][tempnr] = temperature[nr][STORED_ACT_VAL];
    }

  }
  active_sensor = AUSSEN;

  fade_out(MAX_CONTRAST);
  display.clear();
  display.setContrast(MAX_CONTRAST);
  

  
}

/*----------------------------------
 * -------------MAIN----------------
 -----------------------------------*/
void loop() {
  actualTime = millis();
  if( actualTime -lastSensorRead >= MIN_WAIT_TIME_MS )
  {
    // sensors are read after a specified time in milliseconds ... 
    if( actualTime - lastAllSensRead >= STORE_INTERVAL_IN_MS){
      lastAllSensRead = millis();
      for(int sens_nr=0; sens_nr < nr_sensors; sens_nr++){      
        readTempHum( sens_nr, temperature, humidity, heat, ALL_TEMPS, lastAllSensRead );
      }
      if(screen != WELCOME){
        lastscreen = screen;
        screen = GRAPH_OUTER_TEMP;
      } 
    }
    else{
      for(int sens_nr=0; sens_nr < nr_sensors; sens_nr++){            
        readTempHum( sens_nr, temperature, humidity, heat, ONLY_ACTUAL_TEMP,lastAllSensRead);
      }
    }
    lastSensorRead = millis();
  }
  
  motion_detection(&pirState,&lastmotion);
  
  if(pirState == MOTION){
    display.displayOn();
  }
  else
  {
    display.displayOff();
  }
  
  /*for(int sens_nr=0; sens_nr < nr_sensors; sens_nr++){                
    Serial.print( "TMP:");
    Serial.print( temperature[sens_nr][STORED_ACT_VAL]);
    Serial.print( "  HUM:");
    Serial.print( humidity[sens_nr][STORED_ACT_VAL]);
    Serial.print( "   HEAT:");
    Serial.println( heat[sens_nr][STORED_ACT_VAL]);
  }*/

  set_weather_state(temperature , humidity, heat, &weather);
  //Serial.print( "In main  weather.iceornoice:");
  //Serial.println(weather.iceornoice);

  switch(screen){
    case WELCOME:
      lastscreen = screen;
      screen = MAIN;
      break;
    case MAIN:
      draw_main_screen( temperature , humidity, heat, DISPLAY_TITLE, weather);
      break;
    case GRAPH_OUTER_TEMP:
      if((lastSensorRead - lastAllSensRead)>30000){
         screen = lastscreen;
         lastscreen = GRAPH_OUTER_TEMP;
      }
      if(STORE_INTERVAL_IN_MS > 600000){ //only play music if the time between the measurements is big enough (10mins in this case))
        playMusic(); 
      }
      graph.draw_valdiagscreen(display,temperature[active_sensor][STORED_ACT_VAL], temperature[active_sensor][STORED_ACT_CNT], temperature[active_sensor], temperature[active_sensor][STORED_ALL_SUM], DISPLAY_TITLE[active_sensor], max_scale_x_val, max_scale_unit);
      break;
  }


}

/*----------------------------------
 * ----------FUNCTIONS-------------
 -----------------------------------*/
//void clock


 
void motion_detection(motion_e* pirState, unsigned long* lastmotion){
  int val = digitalRead(PIR);  // read input value
  if (val == HIGH) {            // check if the input is HIGH
    *lastmotion = millis(); 
    //Serial.println("Motion detector flags a motion! (HIGH)");
    if (*pirState == NOMOTION) {
        // we have just turned on
        //Serial.println("New Motion detected!");
        *pirState = MOTION;
    }
  } else {
    //Serial.println("Motion detector flags no motion! (LOW)");
    if (*pirState == MOTION){// we have just turned off
      if(millis() - *lastmotion > MOTION_WAIT_BEFORE_SWITCH_MS){
        //Serial.println("Motion ended!");
        *pirState = NOMOTION;
      }
    }
  }
}

void set_weather_state( /* in */ float temp[][STORED_ALL_INFO],
                        /* in */ float hum[][STORED_ALL_INFO],
                        /* in */ float heat[][STORED_ALL_INFO],
                        /* bidi */ wstate* weather)
{

  //this function set all the states of the weather we have - icy,wet,dry,day or night ... depending on the data we have
  float atemp = temp[AUSSEN][STORED_ACT_VAL];
  float ahum = hum[AUSSEN][STORED_ACT_VAL];

  /*Serial.print( "Aussen für States --- TMP:");
  Serial.print( atemp);
  Serial.print( "  HUM:");
  Serial.print( ahum);
  //Serial.print( "   HEAT:");
  //Serial.println( heat[sens_nr][STORED_TEMPS_ACT_TEMP]);
  Serial.print( "Before weather.iceornoice:");
  Serial.println(weather->iceornoice);
  */
  
  // ice is not only depending on temperature - humidity is also a detail of it
  float icehystwarntemp = weather->icewarntemp;
  float icehystwarnhum  = weather->icewarnhum;
  if(weather->iceornoice == ICE){
    icehystwarntemp = icehystwarntemp + weather->icewarntemp * weather->icewarnhyst;
    icehystwarnhum  = icehystwarnhum  - weather->icewarnhum  * weather->icewarnhyst;
  }
  /*
  Serial.print( "Icwarn temp (below is icy, above is not icy): Temp");
  Serial.print(icehystwarntemp);
  Serial.print(" Hum ");
  Serial.println(icehystwarnhum);
  */
  if(atemp > icehystwarntemp || ahum < icehystwarnhum  ){
    //this is quite warm - its not icy
    //Serial.println("EIS!!!! ist  weg!!!");
    weather->iceornoice = NOICE;
  }
  else
  {
    //check if there was noice before - if so we have to display a warning and play a tone
    if(weather->iceornoice == NOICE){
      weather->timeofice = millis();
    }
    //Serial.println("EIS!!!!");
    weather->iceornoice = ICE;
  }

  //Serial.print( "After weather.iceornoice:");
  //Serial.println(weather->iceornoice);
  
  // wet or dry ?
  float wethysthum  = weather->wethum;
  if(weather->wetordry == WET){
    wethysthum  = wethysthum - weather->wethum*weather->wethumhyst;
  }
  if(ahum > wethysthum){
    weather->wetordry = WET;
  }
  else
  {
    weather->wetordry = DRY;
  }
}


void draw_main_screen(  /* in */ float temp[][STORED_ALL_INFO],
                        /* in */ float hum[][STORED_ALL_INFO],
                        /* in */ float heat[][STORED_ALL_INFO],
                        /* in */ const String title[],
                        /* in */ wstate weather)
{
  unsigned long current_millis = millis();
  
  //default screen ...
  main_sceen_s show;
  show.vwpic = true;
  show.mountpic = true;
  show.hum = false;
  show.temp = true;
  show.title = true;  
  show.prio_icewarn = false;
  show.icewarn = false;
  show.progressbar = true;

  //--- set shown values ---
  // ICE!
  if(weather.iceornoice == ICE){
    if((current_millis - weather.timeofice)<5000){
      show.prio_icewarn = true;
      show.vwpic = false;
      show.mountpic = false;
      show.hum = false;
      show.temp = false;
      show.title = false;  
      show.progressbar = false;
    } else {
      show.icewarn = true;
    }
  }
  
  //--- Picture ---
  int pic_pos_x = 15;
  int pic_pos_y = 0;
  int pic_upos_x = 0;
  int pic_upos_y = 0;
  int pic_mpos_x = 0;
  int pic_mpos_y = 10;
  int pic_lpos_x = 0;
  int pic_lpos_y = 45;
  
  //--- AUSSEN --- 
  int ta_int_pos_x = 40;
  int ta_int_pos_y = 20;
  int atitle_int_pos_x = ta_int_pos_x - 20;
  int atitle_int_pos_y = ta_int_pos_y + 20;
  // --- INNEN --
  int ti_int_pos_x = 118;
  int ti_int_pos_y = 20;  
  int ititle_int_pos_x = ti_int_pos_x - 15;
  int ititle_int_pos_y = ti_int_pos_y - 6;

  //----------------------------------
 
  display.clear();
  if(show.prio_icewarn){ 
      //play the tone and display the warning
      display.drawXbm(0,0,gross_glatteis_width, gross_glatteis_height,gross_glatteis_bits);
  }
  if(show.vwpic){
    //Bully
    display.drawXbm(pic_pos_x,pic_pos_y,VWBulli_width, VWBulli_height,VWBulli_bits);
  }
  if(show.mountpic){
    //mountains
    display.drawXbm(pic_mpos_x,pic_mpos_y,berge_width, berge_height,berge_bits);
  }
  if(show.icewarn){
    //ice
    display.drawXbm(pic_lpos_x,pic_lpos_y,glatteis_width, glatteis_height,glatteis_bits);    
  }
  if(show.title){ 
    String atitle = title[AUSSEN];
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(Dialog_plain_8);
    display.drawString(atitle_int_pos_x,atitle_int_pos_y,atitle);
  }
  if(show.temp){
    float atemp = temp[AUSSEN][STORED_ACT_VAL];
    if(atemp == NULL){atemp = -99.9;}
    int int_atemp = int(atemp);
    int half_atemp = abs((atemp - int_atemp)*10);   
    display.setTextAlignment(TEXT_ALIGN_RIGHT);  
    display.setFont(Dialog_bold_22);
    display.drawString(ta_int_pos_x,ta_int_pos_y, String(int_atemp));
    display.setTextAlignment(TEXT_ALIGN_LEFT);    
    display.setFont(Dialog_plain_8);
    display.drawString(ta_int_pos_x,  ta_int_pos_y+2, "°C");  
    display.setFont(Dialog_plain_12);
    display.drawString(ta_int_pos_x-1,  ta_int_pos_y+9, "," + String(half_atemp));
  }
  if(show.hum){
    float ahum = hum[AUSSEN][STORED_ACT_VAL];
    if(ahum == NULL){ahum = -99.9;}
    int int_ahum = int(ahum+0.5);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(Dialog_plain_12);
    display.drawString(ta_int_pos_x+2,  ta_int_pos_y-12, String(int_ahum));
    display.setTextAlignment(TEXT_ALIGN_LEFT); 
    display.setFont(Dialog_plain_8);   
    display.drawString(ta_int_pos_x+3,  ta_int_pos_y-10, "%");
  }
  
  if(show.title){
    String ititle = title[INNEN];        
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(Dialog_plain_8);
    display.drawString(ititle_int_pos_x,ititle_int_pos_y,ititle );
  }
  if(show.temp){
    float itemp = temp[INNEN][STORED_ACT_VAL];
    if(itemp == NULL){itemp = -99.9;}
    int int_itemp = int(itemp);
    int half_itemp = abs((itemp - int_itemp)*10);     
    display.setTextAlignment(TEXT_ALIGN_RIGHT);  
    display.setFont(Dialog_bold_22);
    display.drawString(ti_int_pos_x,ti_int_pos_y, String(int_itemp));
    display.setTextAlignment(TEXT_ALIGN_LEFT);    
    display.setFont(Dialog_plain_8);  
    display.drawString(ti_int_pos_x,ti_int_pos_y+2, "°C");
    display.setFont(Dialog_plain_12);
    display.drawString(ti_int_pos_x-1,ti_int_pos_y+9,"," + String(half_itemp));
  }
  if(show.hum){
    float ihum = hum[INNEN][STORED_ACT_VAL];
    if(ihum == NULL){ihum = -99.9;}
    int int_ihum = int(ihum+0.5);    
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(Dialog_plain_10);
    display.drawString(ti_int_pos_x+2,ti_int_pos_y-12, String(int_ihum));
    display.setTextAlignment(TEXT_ALIGN_LEFT);  
    display.setFont(Dialog_plain_8);  
    display.drawString(ti_int_pos_x+3,ti_int_pos_y-10, "%");
  }

  if(show.progressbar){
    float progress_step = 128 * ((current_millis - temp[AUSSEN][STORED_LAST_TIME]) / temp[AUSSEN][STORED_INTERVAL]);
    display.drawLine(0,63,int(progress_step),63);
  }
    
  display.display();
}

void fade_out(char actual_contrast){
  for(int contrast = actual_contrast; contrast>=MIN_CONTRAST; contrast--){
    display.setContrast(contrast);
    display.display();
  }
}

void fade_in(char actual_contrast){
  for(int contrast = actual_contrast; contrast<=MAX_CONTRAST; contrast++){
    display.setContrast(contrast);
    display.display();
  }
}

void readTempHum( /* in  */ int sensor,
                  /* out */ float temp[][STORED_ALL_INFO],
                  /* out */ float hum[][STORED_ALL_INFO],
                  /* out */ float heat[][STORED_ALL_INFO],
                  /*  in */ int   all_temps,
                  /*  in */ unsigned long lastread)
{
  float act_hum = 0;
  float act_temp = 0;
  float act_heat = 0;
  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)

  act_hum     = (float)dhtsensor[sensor].readHumidity();      // read the indoor humidity
  act_temp    = (float)dhtsensor[sensor].readTemperature();   // read the indoor tempeature in celcius
  act_heat    = (float)dhtsensor[sensor].computeHeatIndex(act_temp, act_hum, false);;   // compute the heat index in celcius

  // the actual value will be set to index STORED_TEMPS_ACT_TEMP value ... 
  temp[sensor][STORED_ACT_VAL] = act_temp;
  hum[sensor][STORED_ACT_VAL]  = act_hum;
  heat[sensor][STORED_ACT_VAL] = act_heat;

  // if this is not the cycle to store the value in the stored field - store the value needed to calculate a mean value later ...
  temp[sensor][STORED_ACT_CNT]++;
  temp[sensor][STORED_ACT_SUM] = temp[sensor][STORED_ACT_SUM] + act_temp;
  hum[sensor][STORED_ACT_CNT]++;
  hum[sensor][STORED_ACT_SUM] = hum[sensor][STORED_ACT_SUM] + act_hum;
  heat[sensor][STORED_ACT_CNT]++;
  heat[sensor][STORED_ACT_SUM] = heat[sensor][STORED_ACT_SUM] + act_heat;

  if(all_temps == ALL_TEMPS)
  {
    for(int nr=(STORED_FIRST+STORED_VALS-1);nr>STORED_FIRST;nr--)
    {
      temp[sensor][nr] = temp[sensor][nr-1];
      hum[sensor][nr] = hum[sensor][nr-1];
      heat[sensor][nr] = heat[sensor][nr-1];
    }

    temp[sensor][STORED_FIRST] = temp[sensor][STORED_ACT_SUM]/temp[sensor][STORED_ACT_CNT];
    hum[sensor][STORED_FIRST]  =  hum[sensor][STORED_ACT_SUM]/hum[sensor][STORED_ACT_CNT];
    heat[sensor][STORED_FIRST] = heat[sensor][STORED_ACT_SUM]/heat[sensor][STORED_ACT_CNT];

    temp[sensor][STORED_ACT_CNT] = 0;
    temp[sensor][STORED_ACT_SUM] = 0;
    hum[sensor][STORED_ACT_CNT]  = 0;
    hum[sensor][STORED_ACT_SUM]  = 0;
    heat[sensor][STORED_ACT_CNT] = 0;
    heat[sensor][STORED_ACT_SUM] = 0;

    // store that we now have another new real value as  long as the value is smaller than the number of stored values ... 
    temp[sensor][STORED_ALL_SUM] = (temp[sensor][STORED_ALL_SUM]<STORED_VALS)? temp[sensor][STORED_ALL_SUM]+1:STORED_VALS;
    hum[sensor][STORED_ALL_SUM] = (hum[sensor][STORED_ALL_SUM]<STORED_VALS)? hum[sensor][STORED_ALL_SUM]+1:STORED_VALS;
    heat[sensor][STORED_ALL_SUM] = (heat[sensor][STORED_ALL_SUM]<STORED_VALS)? heat[sensor][STORED_ALL_SUM]+1:STORED_VALS;

    // store the time of the read of the sensors 
    temp[sensor][STORED_LAST_TIME] = lastread;
    hum [sensor][STORED_LAST_TIME] = lastread;
    heat[sensor][STORED_LAST_TIME] = lastread;
  }
 
}

void playMusic() {
  // iterate over the notes of the melody:
  // notes in the melody:
  int melody[] = {
    NOTE_C5, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_B3, NOTE_B4, NOTE_C5
  };

  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int noteDurations[] = {
    4, 8, 8, 4, 4, 4, 4, 4
  };
  
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    int noteDuration = 1000 / noteDurations[thisNote];
    myTone(BUZZER, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    myNoTone(BUZZER);
    Serial.print("Note:");
    Serial.print(melody[thisNote]);
    Serial.print(" Dauer:");
    Serial.print(noteDuration);
    Serial.print(" Pause:");
    Serial.println(pauseBetweenNotes);
  }
}

void myTone(uint8_t _pin, unsigned int frequency, unsigned long duration) {
  pinMode (_pin, OUTPUT );
  analogWriteFreq(frequency);
  analogWrite(_pin,500);
  delay(duration);
  analogWrite(_pin,0);
}

void myNoTone(uint8_t _pin) {
  pinMode (_pin, OUTPUT );
  analogWrite(_pin,0);
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
    //Serial.println("Response sent to client");

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
  //Serial.println("New client tries to connect ...");
  unsigned long ultimeout = millis()+250;
  while(!tmp_client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
    //Serial.println("Client connection time-out!");
    return; 
  }
  
  // Read the first line of the request
  String sRequest = tmp_client.readStringUntil('\r');
  //Serial.print("Client request is: ");
  //Serial.println(sRequest);
  tmp_client.flush();
  
  // stop client, if request is empty
  if(sRequest=="")
  {
    //Serial.println("Client sends empty request! - stopping client connection");
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
      //Serial.print("Clients sends cmd:");
      //Serial.println(sCmd);
    }
  }
  else
  {
    //Serial.print("Clients sends no cmd.");
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

    //Serial.print("Start executing of cmds:");
    //Serial.println(sCmd);
    
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





