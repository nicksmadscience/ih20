
#include <WiFiUdp.h>
//#include <Adafruit_NeoPixel.h>
#include <string.h>
#include <ESP8266WiFi.h>
// #include "TickTwo.h"

#include "ESP8266TimerInterrupt.h"             //https://github.com/khoih-prog/ESP8266TimerInterrupt
#include "ESP8266_ISR_Timer.hpp" 

#include "FastLED.h"

#define COLORMODE_RGB 0
#define COLORMODE_RGBW 1
#define COLORMODE_BGR 2
#define COLORMODE_GRB 3



// ******** configuration woo ********

uint8_t iplast = 207;
const uint16_t startChannel = 375;
const uint8_t chaserUniverse = 0;
const uint8_t pixelUniverse = 1;






const char* ssid     = "Indy Hall";
const char* password = "coworking";

IPAddress ip(192, 168, 0, iplast);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 254, 0);





// const char* ssid = "marshmallowPrime";
// const char* password = "marshmarsh6";

// IPAddress ip(10, 0, 0, iplast);
// IPAddress gateway(10, 0, 0, 1);
// IPAddress subnet(255, 255, 255, 0);


const uint8_t pin = D5;

#define NUM_LEDS 50

#define COLORMODE COLORMODE_GRB

#define CYCLEENABLED
#define PIXELENABLED
#define DMXENABLED
// #define FAKEWHITE





// ******** end configuration ********


uint8_t chaseEnabled = true;
uint8_t standalone = false;


#if COLORMODE == COLORMODE_RGBW
#define COLORSPERPIXEL 4
#else
#define COLORSPERPIXEL 3
#endif



#if COLORMODE == COLORMODE_RGBW
#include "FastLED_RGBW.h"
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];
#else
CRGB leds[NUM_LEDS];
#endif



#define MODE_CONSTANT 0
#define MODE_CHASE    1  
#define MODE_GRADIENT 2

uint8_t r1 = 0;
uint8_t g1 = 0;
uint8_t b1 = 0;
uint8_t w1 = 0;
uint8_t r2 = 0;
uint8_t g2 = 0;
uint8_t b2 = 0;
uint8_t w2 = 0;
uint8_t shimmer = 128;
uint8_t chase = 64;
uint8_t twinkle = 64;
uint8_t twinklespecial = 0;
uint8_t macro = 0;


uint8_t shimmerMask[NUM_LEDS][4];
uint8_t chaseMask[8];
// uint8_t twinkleOverlay[NUM_LEDS];
uint8_t twinkleOverlay[NUM_LEDS][4];




bool cycleTime = false;
void IRAM_ATTR cycleGo()
{
  cycleTime = true;
}


uint8_t cycleStep = 0;
uint8_t sparkness = 0;
void cycle()
{
  cycleStep++;

  // shimmer
  for (uint8_t i = 0; i < ((shimmer >> 3) + 1); i++)
    for (uint8_t color = 0; color < 4; color++)
      shimmerMask[random(0, NUM_LEDS)][color] = 255 - (random(0, 255) * shimmer);
    

  // chase
  switch ((cycleStep >> 1) & 7)
  {
    case 0: { uint8_t temp0[] = {255, 255, 255, 255, 255 - chase, 255 - chase, 255 - chase, 255 - chase}; memcpy(chaseMask, temp0, sizeof(temp0)); break; }
    case 1: { uint8_t temp1[] = {255 - chase, 255, 255, 255, 255, 255 - chase, 255 - chase, 255 - chase}; memcpy(chaseMask, temp1, sizeof(temp1)); break; }
    case 2: { uint8_t temp2[] = {255 - chase, 255 - chase, 255, 255, 255, 255, 255 - chase, 255 - chase}; memcpy(chaseMask, temp2, sizeof(temp2)); break; }
    case 3: { uint8_t temp3[] = {255 - chase, 255 - chase, 255 - chase, 255, 255, 255, 255, 255 - chase}; memcpy(chaseMask, temp3, sizeof(temp3)); break; }
    case 4: { uint8_t temp4[] = {255 - chase, 255 - chase, 255 - chase, 255 - chase, 255, 255, 255, 255}; memcpy(chaseMask, temp4, sizeof(temp4)); break; }
    case 5: { uint8_t temp5[] = {255, 255 - chase, 255 - chase, 255 - chase, 255 - chase, 255, 255, 255}; memcpy(chaseMask, temp5, sizeof(temp5)); break; }
    case 6: { uint8_t temp6[] = {255, 255, 255 - chase, 255 - chase, 255 - chase, 255 - chase, 255, 255}; memcpy(chaseMask, temp6, sizeof(temp6)); break; }
    case 7: { uint8_t temp7[] = {255, 255, 255, 255 - chase, 255 - chase, 255 - chase, 255 - chase, 255}; memcpy(chaseMask, temp7, sizeof(temp7)); break; }
  }


  // diminish existing sparkles

  if (macro < 32)
    sparkness = lerp(2, 30, twinkle / 255.0);  // mid fadeout
  if (macro >= 32 && macro < 64)
    sparkness = lerp(2, 2, twinkle / 255.0);  // slow fadeout
  else if (macro >= 64 && macro < 96)
    sparkness = lerp(2, 100, twinkle / 255.0);  // fast fadeout 
  else if (macro >= 96 && macro < 128)
    sparkness = lerp(2, 1000, twinkle / 255.0);  // strobe fadeout
    
  for (uint16_t pixel = 0; pixel < NUM_LEDS; pixel++)
  {
    for (uint8_t color = 0; color < 4; color++)
    {
      if (twinkleOverlay[pixel][color] < sparkness)
        twinkleOverlay[pixel][color] = 0;
      else
        twinkleOverlay[pixel][color] = twinkleOverlay[pixel][color] - sparkness;
    }

  }

  // begin new sparkle
  if (random(0, 255) < twinkle)
  {
    uint8_t r = random(0, NUM_LEDS);

    // colorful
    if (twinkle > 200)
    {
      twinkleOverlay[r][0] = random(0, 255);
      twinkleOverlay[r][1] = random(0, 255);
      twinkleOverlay[r][2] = random(0, 255);
      twinkleOverlay[r][3] = random(0, 255);
    }
    
    // twinklespecials
    else
    {
      // xmas
      if (twinklespecial >= 32 && twinklespecial < 64)
      {
        switch (random(0, 3))
        {
          case 0: twinkleOverlay[r][0] = 255; break;
          case 1: twinkleOverlay[r][1] = 255; break;
          case 2: twinkleOverlay[r][0] = 255; twinkleOverlay[r][1] = 255; twinkleOverlay[r][2] = 255; twinkleOverlay[r][3] = 255; break;
          default: break;
        }
      }

      // spooky
      else if (twinklespecial >= 64 && twinklespecial < 96)
      {
        switch (random(0, 10))
        {
          case 0: 
          case 1:
          case 2:
          case 3: twinkleOverlay[r][0] = 255; twinkleOverlay[r][1] = 128; twinkleOverlay[r][2] = 0; break;
          case 4:
          case 5:
          case 6:
          case 7: twinkleOverlay[r][0] = 255; twinkleOverlay[r][1] = 160; twinkleOverlay[r][2] = 0; break;
          case 8: twinkleOverlay[r][0] = 160; twinkleOverlay[r][1] = 0; twinkleOverlay[r][2] = 255; break;
          case 9: twinkleOverlay[r][0] = 0; twinkleOverlay[r][1] = 128; twinkleOverlay[r][2] = 0; break;
          default: break;
        }
      }

      // indy hall
      else if (twinklespecial >= 96 && twinklespecial < 128)
      {
        switch (random(0, 3))
        {
          case 0: twinkleOverlay[r][0] = 255; twinkleOverlay[r][1] = 255; break;
          case 1: twinkleOverlay[r][0] = 128; twinkleOverlay[r][2] = 255; break;
          case 2: twinkleOverlay[r][0] = 255; twinkleOverlay[r][1] = 255; twinkleOverlay[r][2] = 255; twinkleOverlay[r][3] = 255; break;
          default: break;
        }
      }

      // autumnal
      else if (twinklespecial >= 128 && twinklespecial < 160)
      {
        twinkleOverlay[r][0] = random(0, 255);
        twinkleOverlay[r][1] = random(0, 128);
      }

      // go birds
      else if (twinklespecial >= 160 && twinklespecial < 192)
      {
        switch(random(0, 2))
        {
          case 0: twinkleOverlay[r][1] = 255; break;
          case 1: twinkleOverlay[r][0] = 255; twinkleOverlay[r][1] = 255; twinkleOverlay[r][2] = 255; twinkleOverlay[r][3] = 255; break;
        }
      }

      // winter
      else if (twinklespecial >= 192 && twinklespecial < 224)
      {
        switch(random(0, 2))
        {
          case 0: twinkleOverlay[r][2] = 255; break;
          case 1: twinkleOverlay[r][0] = 255; twinkleOverlay[r][1] = 255; twinkleOverlay[r][2] = 255; twinkleOverlay[r][3] = 255; break;
        }
      }

      // spring
      else if (twinklespecial >= 224)
      {
        twinkleOverlay[r][0] = random(0, 128);
        twinkleOverlay[r][1] = random(0, 255);
        twinkleOverlay[r][2] = random(0, 64);
      }

      // rainbow / pride
      else if (twinklespecial == 255)
      {
        switch (random(0, 6))
        {
          case 0: twinkleOverlay[r][0] = 255; break;
          case 1: twinkleOverlay[r][0] = 255; twinkleOverlay[r][1] = 128; break;
          case 2: twinkleOverlay[r][0] = 255; twinkleOverlay[r][1] = 255; break;
          case 3: twinkleOverlay[r][1] = 255; break;
          case 4: twinkleOverlay[r][2] = 255; break;
          case 5: twinkleOverlay[r][0] = 255; twinkleOverlay[r][2] = 255; break;
          default: break;
        }
      }

      // regular (white)
      else
        for (uint8_t color = 0; color < 4; color++)
          twinkleOverlay[r][color] = 255;
    }

    // turbo
    if (twinkle > 250)
    {
      r = random(0, NUM_LEDS);
      for (uint8_t color = 0; color < 4; color++)
      twinkleOverlay[r][color] = 255;
    }

  }
    

  for (uint16_t pixel = 0; pixel < NUM_LEDS; pixel++)
    {
      float red_f   = 
        lerp((float)r1, (float)r2, (float)pixel / (float)NUM_LEDS)
         * ((float)shimmerMask[pixel][0] / 255.0)
         * ((float)chaseMask[pixel & 7] / 255.0);

      float green_f   = 
        lerp((float)g1, (float)g2, (float)pixel / (float)NUM_LEDS)
         * ((float)shimmerMask[pixel][1] / 255.0)
         * ((float)chaseMask[pixel & 7] / 255.0);

      float blue_f   = 
        lerp((float)b1, (float)b2, (float)pixel / (float)NUM_LEDS)
         * ((float)shimmerMask[pixel][2] / 255.0)
         * ((float)chaseMask[pixel & 7] / 255.0);

      float white_f   = 
        lerp((float)w1, (float)w2, (float)pixel / (float)NUM_LEDS)
         * ((float)shimmerMask[pixel][3] / 255.0)
         * ((float)chaseMask[pixel & 7] / 255.0);

      red_f = lerp(red_f, 255.0, (float)twinkleOverlay[pixel][0] / 255.0);
      green_f = lerp(green_f, 255.0, (float)twinkleOverlay[pixel][1] / 255.0);
      blue_f = lerp(blue_f, 255.0, (float)twinkleOverlay[pixel][2] / 255.0);
      white_f = lerp(white_f, 255.0, (float)twinkleOverlay[pixel][3] / 255.0);

      uint8_t red = (uint8_t)red_f;
      uint8_t green = (uint8_t)green_f;
      uint8_t blue = (uint8_t)blue_f;
      uint8_t white = (uint8_t)white_f;

      #if COLORMODE==COLORMODE_RGBW
      leds[pixel].r = red;
      leds[pixel].g = green;
      leds[pixel].b = blue;
      leds[pixel].w = white;
      #elif COLORMODE==COLORMODE_RGB
      leds[pixel].setRGB(red, green, blue);
      #elif COLORMODE==COLORMODE_BGR
      leds[pixel].setRGB(blue, green, red);
      #elif COLORMODE==COLORMODE_GRB
      leds[pixel].setRGB(green, red, blue);
      #endif
    }

    FastLED.show();
}





// Select a Timer Clock
#define USING_TIM_DIV1                false           // for shortest and most accurate timer
#define USING_TIM_DIV16               true           // for medium time and medium accurate timer
#define USING_TIM_DIV256              false            // for longest timer but least accurate. Default

// Init ESP8266 only and only Timer 1
ESP8266Timer ITimer;

#define TIMER_INTERVAL_MS        1000

#define PACKET_LENGTH 530
WiFiUDP Udp;
unsigned int localUdpPort = 6454;
char incomingPacket[PACKET_LENGTH];

#ifdef DMXENABLED
uint32_t lastDMXSend; // for auto sending packet after loss of artnet
char channels[512];
#endif


void sendDMX()
{
  // Serial.print(".");

  #ifdef DMXENABLED
  lastDMXSend = millis();

  Serial1.flush();
  Serial1.begin(90000, SERIAL_8N2);
  while(Serial1.available()) Serial1.read();
  // send the break as a "slow" byte
  Serial1.write(0);
  // switch back to the original baud rate
  Serial1.flush();
  Serial1.begin(250000, SERIAL_8N2);
  while(Serial1.available()) Serial1.read();

  Serial1.write(0); // Start-Byte
  for (int i = 0; i < 512; i++)
    Serial1.write(channels[i]);

    #endif
}




float lerp(float x1, float x2, float alpha)
{
  float diff = x2 - x1;
  return x1 + (diff * alpha);
}



uint8_t standaloneColor = 0;
void incrementColor()
{
  standaloneColor = standaloneColor == 1 ? 0 : standaloneColor + 1;

  switch (standaloneColor)
  {
    default:
    case 0: r1 = 255; g1 = 0; b1 = 0; break;
    case 1: r1 = 255; g1 = 128; b1 = 0; break;
    case 2: r1 = 255; g1 = 255; b1 = 0; break;
    case 3: r1 = 128; g1 = 255; b1 = 0; break;
    case 4: r1 = 0; g1 = 255; b1 = 0; break;
    case 5: r1 = 0; g1 = 255; b1 = 128; break;
    case 6: r1 = 0; g1 = 255; b1 = 255; break;
    case 7: r1 = 0; g1 = 128; b1 = 255; break;
    case 8: r1 = 0; g1 = 0; b1 = 255; break;
    case 9: r1 = 128; g1 = 0; b1 = 255; break;
    case 10: r1 = 255; g1 = 0; b1 = 255; break;
    case 11: r1 = 255; g1 = 0; b1 = 128; break;
    case 12: r1 = 255; g1 = 255; b1 = 255; break;
  }

  r2 = r1;
  g2 = g1;
  b2 = b1;
  w2 = w1;
}


uint16_t i = 0;
uint16_t pixel = 0;
uint8_t gammalut[256];


void setup()
{
  Serial.begin(74800);
  delay(10);

  #ifdef DMXENABLED
  Serial1.begin(250000, SERIAL_8N2); // IT'S GPIO2 / D4, FOR THE LOVE OF GOD IT'S GPIO2 / D4.  NOT TX.
  #endif

  for (i = 0; i < NUM_LEDS; i++)
  {
    for (uint8_t color = 0; color < 4; color++)
    {
      shimmerMask[i][color] = 255;
      twinkleOverlay[i][color] = 0;
    }
  }

  for (i = 0; i < 8; i++)
  {
    chaseMask[i] = 255;
  }

  #if COLORMODE==COLORMODE_RGBW
  FastLED.addLeds<WS2812B, pin, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  #else
  FastLED.addLeds<WS2811, pin>(leds,  NUM_LEDS);
  #endif

  // generate gamma lut
  for (uint16_t i = 0; i < 256; i++)
  {
    gammalut[i] = (uint8_t)((pow((float)i / 255.0, 1.0 / .5)) * 255.0);
    Serial.print(gammalut[i]);
    Serial.print(" ");
  }
  Serial.println();


  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  uint8_t attempts = 0;

  while ((WiFi.status() != WL_CONNECTED) && attempts < 100)
  {
    delay(250);
    switch (WiFi.status())
    {
      case WL_CONNECTED: Serial.println("connected to a WiFi network"); break;
      case WL_NO_SHIELD: Serial.println("no WiFi shield is present"); break;
      case WL_IDLE_STATUS: Serial.println("WiFi.begin() is called and remains active until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a connection is established (resulting in WL_CONNECTED);");  break;
      case WL_NO_SSID_AVAIL: Serial.println("no SSID are available;"); break;
      case WL_SCAN_COMPLETED: Serial.println("the scan networks is completed;"); break;
      case WL_CONNECT_FAILED: Serial.println("the connection fails for all the attempts;"); break;
      case WL_CONNECTION_LOST: Serial.println("the connection is lost;"); break;
      case WL_DISCONNECTED: Serial.println("disconnected from a network; "); break;
      default: Serial.println(WiFi.status());
    }

    attempts++;
    Serial.println(attempts);

    for (pixel = 0; pixel < NUM_LEDS; pixel++)
      leds[pixel].g = ((attempts + pixel) % 8) * 32;

    FastLED.show();
  }

  if (attempts >= 100)
  {
    standalone = true;
    Serial.println("could not connect; reverting to standalone mode");

    for (i = 0; i < 5; i++)
    {
      for (pixel = 0; pixel < NUM_LEDS; pixel++)
        leds[pixel].g = 255;


      FastLED.show();
      delay(180);

      for (pixel = 0; pixel < NUM_LEDS; pixel++)
        leds[pixel].g = 0;

      FastLED.show();
      delay(180);

    }
  }
  else
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Udp.begin(localUdpPort);
  }

  switch (iplast % 3)
  {
    default:
    case 0: r1 = 255; g1 = 128; b1 = 0; w1 = 0; break;
    case 1: r1 = 0; g1 = 0; b1 = 255; w1 = 0; break;
    case 2: 
      #if COLORSPERPIXEL == 3
      r1 = 255; g1 = 255; b1 = 255; w1 = 0;
      #else
      r1 = 0; g1 = 0; b1 = 0; w1 = 255;
      #endif
      break;
  }

  r2 = r1;
  g2 = g1;
  b2 = b1;
  w2 = w1;

    // Interval in microsecs
  if (ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 25, cycleGo))
  {
    uint32_t lastMillis = millis();
    Serial.print(F("Starting  ITimer OK, millis() = ")); Serial.println(lastMillis);
  }
  else
    Serial.println(F("Can't set ITimer correctly. Select another freq. or interval"));
}


uint32_t lastStandaloneButtonMillis = 0;
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;
uint8_t white = 0;
int packetSize;
int len;
uint8_t universe;

uint8_t purge = false;
String lastip;
uint8_t lastUniverse = 0;

void loop() 
{
  if (!standalone)
  {
    packetSize = Udp.parsePacket();
    if (packetSize)
    {
      len = Udp.read(incomingPacket, PACKET_LENGTH);
      universe = incomingPacket[14];
      Serial.printf("Received %d / %d bytes from %s, port %d, universe %d\n", packetSize, len, Udp.remoteIP().toString().c_str(), Udp.remotePort(), universe);

      if (Udp.remoteIP().toString() != lastip)
      {
        Serial.println("receiving art-net from different ip address; pausing for one second");
        delay(1000);

        // purge pending packets
        purge = true;
        while (purge)
        {
          packetSize = Udp.parsePacket();
          purge = packetSize;
          Serial.println("purging packet");
        }
      }
      else
      {
    
        if (len > 0)
          incomingPacket[len] = 0;

        if (universe == chaserUniverse)
        {
          chaseEnabled = true;

          // greatly simplified
          r1 = incomingPacket[startChannel + 17];
          g1 = incomingPacket[startChannel + 18];
          b1 = incomingPacket[startChannel + 19];

          r1 = gammalut[r1];//(uint8_t)((pow((float)r1 / 255.0, 1.0 / .3)) * 255.0);
          g1 = gammalut[g1];//(uint8_t)((pow((float)g1 / 255.0, 1.0 / .3)) * 255.0);
          b1 = gammalut[b1];//(uint8_t)((pow((float)b1 / 255.0, 1.0 / .3)) * 255.0);

          r2 = r1;
          g2 = g1;
          b2 = b1;

          #if COLORSPERPIXEL == 3
          #ifdef FAKEWHITE
          twinkle = incomingPacket[startChannel + 21];
          chase = incomingPacket[startChannel + 22];
          shimmer = incomingPacket[startChannel + 23];
          #else
          twinkle = incomingPacket[startChannel + 20];
          chase = incomingPacket[startChannel + 21];
          shimmer = incomingPacket[startChannel + 22];
          #endif
          #else
          w1 = incomingPacket[startChannel + 20];
          w1 = gammalut[w1];//(uint8_t)((pow((float)w1 / 255.0, 1.3)) * 255.0);
          w2 = w1;
          twinkle = incomingPacket[startChannel + 21];
          chase = incomingPacket[startChannel + 22];
          shimmer = incomingPacket[startChannel + 23];
          #endif

          #ifndef CYCLEENABLED
          for (uint16_t i = 0; i < NUM_LEDS; i++)
          {
            #if COLORMODE==COLORMODE_RGBW
            leds[i].r = r1;
            leds[i].g = g1;
            leds[i].b = b1;
            leds[i].w = w1;
            #elif COLORMODE==COLORMODE_RGB
            leds[i].setRGB(r1, g1, b1);
            #elif COLORMODE==COLORMODE_BGR
            leds[i].setRGB(b1, g1, r1);
            #elif COLORMODE==COLORMODE_GRB
            leds[i].setRGB(g1, r1, b1);
            #endif  
          }
          #endif

          twinklespecial = incomingPacket[517]; // hard-coded at channel 500 because i don't feel like repatching everything
          macro = incomingPacket[518];

          Serial.print("r: ");
          Serial.print(r1);
          Serial.print("  g: ");
          Serial.print(g1);
          Serial.print("  b: ");
          Serial.print(b1);
          #if COLORSPERPIXEL == 4
          Serial.print("  w: ");
          Serial.print(w1);
          #endif
          Serial.print("  shimmer: ");
          Serial.print(shimmer);
          Serial.print("  chase: ");
          Serial.print(chase);
          Serial.print("  twinkle: ");
          Serial.print(twinkle);
          Serial.print("  twinklespecial: ");
          Serial.print(twinklespecial);
          Serial.print("  macro: ");
          Serial.println(macro);


          FastLED.show();

          #ifdef DMXENABLED
          for (i = 0; i < 512; i++)
            channels[i] = incomingPacket[18 + i];
          #endif

        }
        #ifdef PIXELENABLED
        else if (universe == pixelUniverse)
        {
          chaseEnabled = false; // stay in pixel mode until we get another chase packet
          for (pixel = 0; pixel < NUM_LEDS; pixel++)
          {
            red = incomingPacket[(pixel * COLORSPERPIXEL) + 17];
            green = incomingPacket[(pixel * COLORSPERPIXEL) + 18];
            blue = incomingPacket[(pixel * COLORSPERPIXEL) + 19];

            #if COLORMODE==COLORMODE_RGBW
            white = incomingPacket[(pixel * 4) + 20];
            leds[pixel].r = red;
            leds[pixel].g = green;
            leds[pixel].b = blue;
            leds[pixel].w = white;
            #elif COLORMODE==COLORMODE_RGB
            leds[pixel].setRGB(red, green, blue);
            #elif COLORMODE==COLORMODE_BGR
            leds[pixel].setRGB(blue, green, red);
            #elif COLORMODE==COLORMODE_GRB
            leds[pixel].setRGB(green, red, blue);
            #endif
          }

          Serial.println("pixel packet show");
          FastLED.show();
        }
        #endif
      }

      lastip = Udp.remoteIP().toString().c_str();
    }
  }

  #ifdef CYCLEENABLED
  if (chaseEnabled && cycleTime)
  {
    cycle();
    cycleTime = false;
  }
  #endif

  #ifdef DMXENABLED
  if (millis() >= lastDMXSend + 25)
    sendDMX();
  #endif

  delay(1);

}


