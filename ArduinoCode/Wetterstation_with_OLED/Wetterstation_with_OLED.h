
#ifndef Wetterstation_with_OLED_H
#define Wetterstation_with_OLED_H

//--- Options of the temprature/humidity sensors .... you can as many as connected via a GPIO to the list ... the code will include all of them -----------
//---

const String DISPLAY_TITLE[] = {                                   // titles to display at the OLED when the mesured values of the corresponding sensor is shown, just add more ... 
                                            "Innen",
                                            "Balkon Kueche",
                                            "Balkon WohnZimmer"
                                          };
const int IN_DHTPIN[] = {2,0,14};                                 // GPIO of the nodemcu the temperature/humidity sensors are connected to




//--- Options of the Wireless Webservermodule/ Wifi module ... ---------------------------------------------------------------------------------------------
//---

const char* ssid     = "Temperature";         // SSID the ESP Wifi will be visible
const char* password = "";                    // no password needed so far
#define SERVER_PORT 80                        // server port, IP Address will be 192.168.1.4 ... not sure where it comes from, will need to check that

#endif
