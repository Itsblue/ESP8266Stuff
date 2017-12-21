
#ifndef Wetterstation_with_OLED_H
#define Wetterstation_with_OLED_H

//--- PIR
#define PIR 16          //this is the input of the infrred motion detection device
typedef enum {MOTION, NOMOTION} motion_e;
#define MOTION_WAIT_BEFORE_SWITCH_MS 20000  //if a change has detected from motion to nomotion or the other way arround we will wait this amopunt of time befroe switch to the other state

//--- for RTC timer
const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

//-------------- defines fpr the radio devices NRF24 ---------------------------------------------------------

//#define RADIO_SEL 4   // this 4 for Nano
typedef enum {RADIO0 = 0, RADIO1} radio_type_e;      
#define RF24_CNS D8    // this is 7 for the Nano, D4 for the ESP
#define RF24_CE D4     // this is 8 for the ESP, D3 for the ESP

//--- Buzzer
#define BUZZER D0        // this is the output pin of the buzzer

//------------- Button -------------------------------------
#define BUTTON D9

//--- Options of the temprature/humidity sensors .... you can as many as connected via a GPIO to the list ... the code will include all of them -----------
typedef enum {INNEN, AUSSEN} location_sensor;
const String DISPLAY_TITLE[] = {                                   // titles to display at the OLED when the mesured values of the corresponding sensor is shown, just add more ... 
                                  [INNEN]  = "Innen",
                                  [AUSSEN] = "Aussen"
                                  //[AUSSEN2]="Aussen2"
                               };
                               
const int IN_DHTPIN[] = {
                          [INNEN]  = 2,
                          [AUSSEN] = 0
                          //[AUSSEN2]= 14
                        };                                       // GPIO of the nodemcu the temperature/humidity sensors are connected to

typedef enum {DAY, NIGHT} dayornight_e;
typedef enum {WET,DRY} wetordry_e;
typedef enum {ICE,NOICE} iceornoice_e;
typedef struct weather_state_struct
{
  dayornight_e dayornight;
  wetordry_e wetordry;
  float wethum = 90.0;
  float wethumhyst = 0.1;
  iceornoice_e iceornoice;
  float icewarntemp = 4.0;
  float icewarnhum = 0.0;
  float icewarnhyst = 0.1;
  unsigned long timeofice;
  float heat;
  float humidity; 
  float temperature;
  unsigned long timesecs;
} wstate;

typedef enum {STORED_ACT_VAL,    //actual temperature is stored at this position in the array
              STORED_ACT_CNT,    //number meassured values since last store
              STORED_ACT_SUM,    //sum of all messurements since last store
              STORED_LAST_TIME,  //milli secs the last element was stored
              STORED_INTERVAL,  //interval in millis a store happens
              STORED_ALL_SUM,    //sum of all stored values
              STORED_FIRST       //position of the first stored value - has to be the last element in this enum list
             } stored_e;

//--- main screen options 

typedef enum {WELCOME,MAIN, GRAPH_OUTER_TEMP,GRAPH_INNER_TEMP,GRAPH_OUTER_HUM,GRAPH_INNER_HUM,GRAPH_OUTER_PRESSURE, GRAPH_INNER_PRESSURE,CLOCK,OFF}screen_e; 

typedef struct main_screen_struct
{
  bool vwpic = true;
  bool mountpic = true;
  bool hum = false;
  bool temp = true;
  bool title = true;  
  bool prio_icewarn = false;
  bool icewarn = false;
  bool progressbar = true;
} main_sceen_s;

//--- Options of the Wireless Webservermodule/ Wifi module ... ---------------------------------------------------------------------------------------------
//---

const char* ssid     = "Temperature";         // SSID the ESP Wifi will be visible
const char* password = "";                    // no password needed so far
#define SERVER_PORT 80                        // server port, IP Address will be 192.168.1.4 ... not sure where it comes from, will need to check that
#endif
