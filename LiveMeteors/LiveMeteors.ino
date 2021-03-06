
// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// Released under the GPLv3 license to match the rest of the
// Adafruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        25 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 20 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 25 // Time (in milliseconds) to pause between pixels

/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <ETH.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiSTA.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 13 // pin number is specific to your esp32 board
#endif

const char *copyright_notice = "Sophie Adams, Copyright June 6,2021, All Rights Reserved.";

// UDS buffer for server messages
String msg;

// UDS client messages
const char uds_init[] = {'I', 'N', 'I', 'T', 0xff}; // US init 
const char uds_poll[] = {'P', 'O', 'L', 'L', 0xff}; // We want to use poll mode to get data
const char uds_getc[] = {'G', 'E', 'T', 'C', 0xff}; // Get number of data channels
const char uds_stat[] = {'S', 'T', 'A', 'T', 0xff}; // Let UDS server get ready to start data stream
const char uds_getd[] = {'G', 'E', 'T', 'D', 0xff}; // Poll for data

int available = 0;
unsigned long strike_factor = 2.1;
unsigned int neo_pixel_flash_count = 5;
uint64_t restarts = 0;
uint32_t wifi_connect_retries = 30;

const char *hotspot_ssid      = "Livemeteors";
const char *hotspot_password  = "LivemeteorsLivemeteors";
const char *wifi_ssid         = "wca-goo";
const char *wifi_password     = "pinheads1pinheads1";

// Uncomment next line if using "hot spot"
#define USE_HOTSPOT
#ifdef USE_HOTSPOT
  const char *ssid              = hotspot_ssid;
  const char *password          = hotspot_password;
#else
  const char *ssid              = wifi_ssid;
  const char *password          = wifi_password;
#endif // USE_HOT_SPOT

// SkyPipe server TCP connection information
const char* host = "69.30.241.74";     // livemeteor website server
const int port = 6300;                        // TCP port for skypipe servers
const int skip_message_count = 500;
uint32_t bytes_sent = 0;

unsigned long message_count = 0;
unsigned long signal_aggregate = 0;
unsigned long signal_average = 0;
unsigned long meteor_strikes = 0;
unsigned long randomStrike = 0;
unsigned long randomStrikeCount = 0;
bool randomizerEnabled = false;

// Use WiFiClient class to create TCP connections
WiFiClient client;

int onboard_led_is_on = 0;      

void neo_pixel_flash_on()
{
    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    // Here we're using a moderately bright green color:

//    Serial.print("randomPixel is ");
//    Serial.println(randomPixel);

    uint8_t r = 128;
    uint8_t g = 128;
    uint8_t b = 128;
    uint8_t p = random(0, NUMPIXELS);

    if (randomizerEnabled == true)
    {
      r = random(0, 250);
      g = random(0, 250);
      b = random(0, 250);
    }

    pixels.setPixelColor(p, pixels.Color(r, g, b));
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(DELAYVAL); // Pause before next pass through loop
}

void neo_pixel_flash_off()
{
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(DELAYVAL); // Pause before next pass through loop
}

void neo_pixel_flash()
{
  if (randomizerEnabled == true) {
    Serial.println("randomizerEnabled is True!\n");
  }

  for (int i = 0; i < neo_pixel_flash_count; i++)
  {
    neo_pixel_flash_on();
    neo_pixel_flash_off();
  }
}

void onboard_led_on()
{
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  onboard_led_is_on = 1;      
}

void onboard_led_off()
{
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(2);
  onboard_led_is_on = 0;
}

void onboard_led_flash(int count)
{
  int i = count;
  while(i-- > 0) 
  {
    onboard_led_on();
    delay(250);
    onboard_led_off();
    delay(250);
  }
}

void my_init()
{
//    message_count = 0;
//    signal_aggregate = 0;
//    signal_average = 0;
    available = 0;
    onboard_led_off();
    neo_pixel_flash_off();
}

void my_cleanup()
{
    onboard_led_off();
    neo_pixel_flash_off();
    Serial.print(">>> Closing connection to host => ");
    Serial.println(host);
    client.stop();
    WiFi.disconnect();
}

void setup()
{
    Serial.begin(115200);
    delay(10);
    Serial.flush();
    Serial.println(copyright_notice);

    // init onboard led
    pinMode(LED_BUILTIN, OUTPUT);
   
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  randomSeed(analogRead(0));
}

void my_randomizer()
{
  if(randomizerEnabled == false)
  {
    return;
  }

  onboard_led_flash(5);
  onboard_led_on();
  
  // spin here faking data for awhile, then retry normal path again
  uint32_t strikes = random(5, 15);
  uint32_t randomWaitMs = random(20000, 60000);
 
  Serial.print(">>> RandomizerEnabled => wait time is ");
  Serial.print(randomWaitMs/1000);
  Serial.print(" (seconds), random srikes set to ");
  Serial.println(strikes);
  Serial.flush();
  
 delay(randomWaitMs);
 for (int i = 0; i < strikes; i++)
  {
    neo_pixel_flash();
    onboard_led_flash(1);
  }
}

void loop()
{
    do {
      my_init();

      my_randomizer();
      
      Serial.print(">>> Trying to connect to network => ");
      Serial.println(ssid);
  
      // connect to wifi network first
      WiFi.begin(ssid, password);
      int32_t retry = wifi_connect_retries;
      while (WiFi.status() != WL_CONNECTED) {
          delay(250);      
          if (retry-- <= 0) {
            randomizerEnabled = true;
            goto done;
          }
          
          if ((retry % 10) == 0) 
          {
              Serial.println(); 
          }
          Serial.print(".");
      }
  
      Serial.println();
      onboard_led_on();
      Serial.print(">>> Connected to network => ");
      Serial.println(ssid);
      Serial.print(">>> My IP address is => ");
      Serial.println(WiFi.localIP());
      
      // ==================
          
      delay(500);
     
      Serial.print(">>> Trying to connect to host => ");
      Serial.print(host);
      Serial.print(", port => ");
      Serial.println(port);
      
      if (!client.connect(host, port)) {
          Serial.print(">>> Failed to connect to host => ");
          Serial.println(host);
          randomizerEnabled = true;
          goto done;
      }
  
      randomizerEnabled = false;
      
      onboard_led_on();
      
      Serial.print(">>> Connected to host => ");
      Serial.print(host);
      Serial.print(", port => ");
      Serial.println(port);
  
      // INIT State
      Serial.print(">>> Sending init string to host => ");
      Serial.println(host);    

//      for (int i = 0; i < sizeof(uds_init); i++) {
//        Serial.print(uds_init[i], HEX);
//        Serial.print(" ");
//      }
//      Serial.println();

       bytes_sent = client.write(uds_init, sizeof(uds_init));
       if (bytes_sent != sizeof(uds_init))
       {
        Serial.print(">>> INIT msg bytes sent (not equal to message size): ");
        Serial.println(bytes_sent);
       }

      // Did we get a UDS Ready response?
      do 
      {
        if (available = client.available())
        {
          Serial.print(">>> Bytes available from USD server: ");
          Serial.println(available);
          // read next message from server
          msg = client.readStringUntil(0xff);  
          if (msg.substring(0) == "^^1001")
          {
            // got it!
            Serial.println(">>> UDS Ready message received"); 
            break;
          }
          else
          {
            // ignore msg
            Serial.print(">>> Non-USD Ready message ignored: ");
            Serial.println(msg);            
          }
        } 
        else 
        {
          if (client.connected() == 0) 
          {
            Serial.println(">>> Lost connection to host");
            goto done;
          }
          else
          {
            delay(5);     
          }
        }
      } while(true);

      // POLL for data
      bytes_sent = client.write(uds_poll, sizeof(uds_poll));
      if (bytes_sent != sizeof(uds_poll))
      {
        Serial.print(">>> POLL msg bytes sent (not equal to message size): ");
        Serial.println(bytes_sent);
      }
      
      // Read messages from server 
      do {
          while ((available = client.available()) == 0) {
            delay(5);
            if (client.connected() == 0) {
              Serial.print(">>> Lost my connection to host => ");
              Serial.println(host);
              goto done;
            }
            onboard_led_on();
          } 
  
          // read next message from server
          msg = client.readStringUntil(0xff);
          // data channel
          if (msg.indexOf('#') == 0) 
          {            
            message_count++;
            if ((message_count % 1000) == 0) {
              Serial.print(">>> msg: ");
              Serial.print(message_count);
              Serial.print(", sig avg: ");
              Serial.println(signal_average);
            }
   
            int indexOfLastSpace = msg.lastIndexOf(' ');
            int indexOfLastDot = msg.lastIndexOf('.');
            String valueStr = (msg.substring((indexOfLastSpace + 1), indexOfLastDot));
            int value = valueStr.toInt();
            // should be based upon a recent moving average threshold
            signal_aggregate += value;
            signal_average = signal_aggregate/message_count;
            if ((value > (signal_average * strike_factor)) && (message_count > skip_message_count)) {          
              meteor_strikes++;
              Serial.print(">>> msg no: ");
              Serial.print(message_count);
              Serial.print(", sig value: ");
              Serial.print(value);        
              Serial.print(", sig avg: ");
              Serial.print(signal_average);
              Serial.print(", sig aggregate: ");
              Serial.print(signal_aggregate);
              Serial.print(", meteor strikes: ");
              Serial.println(meteor_strikes);
              // meteor strike!  
              neo_pixel_flash();
            }
          }
          else 
          {
            // ignore msg
            Serial.print(">>> Non data msg ignored: ");
            Serial.println(msg);
          }
        } while (true);
      
done:
      my_cleanup();
      Serial.println(">>> Restarting in a bit...");
      delay(1000);
      restarts++;
      Serial.print(">>> Restart: ");
      Serial.println(restarts);
    } while(true);
    
    return;
}

#ifdef NOTES
To do:
- email webmaster that server is down
- use default hotspot name/password for WiFi connectivity
- sometimes wifi or hotspot connectivity fails and only way to recover is to re-download uProcessor
   - how can I emulate that??
#endif // NOTES
