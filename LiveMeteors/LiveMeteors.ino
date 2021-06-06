
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
#define NUMPIXELS 2 // Popular NeoPixel ring size

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

String msg;
char pipe_init[5];
int available = 0;

char *hotspot_ssid      = "Livemeteors";
char *hotspot_password  = "LivemeteorsLivemeteors";
char *wifi_ssid         = "wca-goo";
char *wifi_password     = "pinheads1pinheads1";
char *ssid              = wifi_ssid;
char *password          = wifi_password;
 
// SkyPipe server TCP connection information
char* host1 = "69.30.241.74";     // livemeteor website server
char* host2 = "166.122.43.82";    // known working (sometimes) skypipe server
char* host3 = "174.131.124.214";  // known working (sometimes) skypipe server
char* host4 = "192.168.1.104";
char* host5 = "151.101.0.153";     // www.boston.com
char* host6 = "173.185.82.85";
char* host = host1; 

int skip_message_count = 500;
const int port = 6300;                        // TCP port for skypipe servers
// const int port = 80;                        // TCP port for skypipe servers

unsigned long message_count = 0;
unsigned long signal_aggregate = 0;
unsigned long signal_average = 0;
unsigned long meteor_strikes = 0;

// Use WiFiClient class to create TCP connections
WiFiClient client;

int led_is_on = 0;      

void neo_flash_on()
{
  // to the count of pixels minus one.
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...

    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    // Here we're using a moderately bright green color:
    pixels.setPixelColor(i, pixels.Color(128, 128, 128));

    pixels.show();   // Send the updated pixel colors to the hardware.

    delay(DELAYVAL); // Pause before next pass through loop
  }
}

void neo_flash_off()
{
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(DELAYVAL); // Pause before next pass through loop
}

void neo_flash()
{
  for (int i = 0; i < 10; i++)
  {
    neo_flash_on();
    neo_flash_off();
  }
}

void led_on()
{
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  led_is_on = 1;      
}

void led_off()
{
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(10);
  led_is_on = 0;
}

void flash_leds(int count)
{
  int i = count;
  while(i-- > 0) 
  {
    led_on();
    delay(500);
    led_off();
    delay(500);
  }
}

void my_init()
{
//    message_count = 0;
//    signal_aggregate = 0;
//    signal_average = 0;
    available = 0;
    
    pipe_init[0] = 'I';
    pipe_init[1] = 'N';
    pipe_init[2] = 'I';
    pipe_init[3] = 'T';
    pipe_init[4] = 0xff;

//    led_off();
    neo_flash_off();
}

void setup()
{
    Serial.begin(115200);
    delay(10);
   
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

void loop()
{
    my_init();
       
    Serial.print(">>> Trying to connect to network => ");
    Serial.println(ssid);

    // connect to wifi network first
    WiFi.begin(ssid, password);
    int retry = 120;
    while (WiFi.status() != WL_CONNECTED) {
        // flash_leds(2);  
        delay(500);      
        if (retry-- < 0) {
          goto done;
        }
        
        if ((retry % 10) == 0) 
        {
            Serial.println(); 
        }
        Serial.print(".");
    }

    Serial.println();
    //led_on();
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
        goto done;
    }

    Serial.print(">>> Connected to host => ");
    Serial.print(host);
    Serial.print(", port => ");
    Serial.println(port);

    Serial.print(">>> Sending init string to host => ");
    Serial.println(host);    
    for (int i = 0; i < sizeof(pipe_init); i++) {
      Serial.print(pipe_init[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Read all the lines of the reply from server and print them to Serial
    do {
        while ((available = client.available()) == 0) {
          // led_off();
          delay(10);
          if (client.connected() == 0) {
            Serial.print(">>> Lost my connection to host => ");
            Serial.println(host);
            goto done;
          }
          // led_on();
        } 

        // read next message from server
        msg = client.readStringUntil(0xff);
        message_count++;
        if ((message_count % 1000) == 0) {
          Serial.print(">>> msg: ");
          Serial.print(message_count);
          Serial.print(", sig avg: ");
          Serial.println(signal_average);
        }
        // data channel
        if (msg.indexOf('#') == 0) {
          int indexOfLastSpace = msg.lastIndexOf(' ');
          int indexOfLastDot = msg.lastIndexOf('.');
          String valueStr = (msg.substring((indexOfLastSpace + 1), indexOfLastDot));
          int value = valueStr.toInt();
          // should be based upon a recent moving average threshold
          signal_aggregate += value;
          signal_average = signal_aggregate/message_count;
          if ((value > (signal_average * 1.5)) && (message_count > skip_message_count)) {          
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
            neo_flash();
          }
        }
      } while (true);
    
done:
    // led_off();
    neo_flash_off();
    Serial.print(">>> Closing connection to host => ");
    Serial.println(host);
    client.stop();
    WiFi.disconnect();
    Serial.println(">>> Restarting in a bit...");
    delay(7500);
    Serial.println(">>> Restarting!");
    return;
}

#ifdef NOTES
To do:
- setup an array of hosts/ports to iterate through until can connect
- if no servers are available, generate random numbers to simulate a meteor strike every randome number time interval
- email webmaster that server is down
- use default hotspot name/password for WiFi connectivity
- sometimes wifi or hotspot connectivity fails and only way to recover is to re-download uProcessor
   - how can I emulate that??
- randomly light up a different neo-pixel per strike
#endif // NOTES
