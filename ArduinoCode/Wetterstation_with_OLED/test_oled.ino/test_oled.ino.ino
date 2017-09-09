#include "U8glib.h"
   
  U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);    // I2C / TWI
   
  void setup()
  {
      u8g.setColorIndex(1); //BW Display
  }
   
  void loop()
  {
    u8g.firstPage();
    while(u8g.nextPage()==1)
    {
      u8g.setFont(u8g_font_4x6);
      u8g.drawStr( 0, 6, "Hello World!");
      u8g.setFont(u8g_font_5x7);
      u8g.drawStr( 0, 13, "Hello World!");
      u8g.setFont(u8g_font_5x8);
      u8g.drawStr( 0, 21, "Hello World!");
      u8g.setFont(u8g_font_6x10);
      u8g.drawStr( 0, 31, "Hello World!");
      u8g.setFont(u8g_font_7x13);
      u8g.drawStr( 0, 44, "Hello World!");
      u8g.setFont(u8g_font_8x13);
      u8g.drawStr( 0, 57, "Hello World!");
    }
    delay(1000);
    u8g.firstPage();
    while(u8g.nextPage()==1)
    {
      delay(1);
    }
    delay(500);
  }
