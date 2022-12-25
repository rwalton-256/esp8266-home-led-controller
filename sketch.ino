#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <EEPROM.h>

// In preferences add the following as additional board manager URL
// https://arduino.esp8266.com/stable/package_esp8266com_index.json
//
// In board manager, search for and install esp8266
//
// Select Arduino->tools->Board
//
// NodeMCU 1.0 (ESP-12E Module)
//   80 Mhz
//   115200
//   4M (3M SPIFFS)
//

// Pin use:
//
// D5 GPIO14 R LEDs on when low
// D6 GPIO12 G LEDs on when low
// D7 GPIO13 B LEDs on when low
//

#define DATA_PIN 16
#define NUM_LEDS 150

char ssid[30] = "";  //  your network SSID (name)
char password[30] = "";       // your network password

CRGB leds[NUM_LEDS];
const CRGB dim_white = CRGB( 10, 10, 10 );
const CRGB dim_red = CRGB( 10, 0, 0 );
const CRGB dim_green = CRGB( 0, 10, 0 );

// Set web server port number to 80
WiFiServer server(80);

String HTTP_req;          // stores the HTTP request
boolean LED_status = 0;   // state of LED2, off by default
boolean LED_status3 = 0;   // state of LED3, off by default

void setup() {
  Serial.begin(115200);

  EEPROM.begin(sizeof(ssid) + sizeof(password));

  EEPROM.get( 0, ssid );
  EEPROM.get( sizeof(ssid), password );

  // Connect to Wi-Fi network with SSID and password
  Serial.print("\r\nConnecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);

  int count = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    count++;
    if( count > 20 )
    {
      Serial.println("\r\nFailed to connect to WiFi!");
      Serial.println("Enter WiFi credentials and power cycle");      
      char c;
      memset( ssid, 0, sizeof( ssid ) );
      memset( password, 0, sizeof( password ) );
      while( Serial.available() )
      {
        Serial.read();
      }
      Serial.println("Enter SSID:");
      uint8_t i = 0;
      while(1)
      {
        if( Serial.available() )
        {
          c = Serial.read();
          if( c == '\r' || c == '\n' )
          {
            break;
          }
          ssid[i++] = c;
        }
      }
      Serial.println(ssid);
      EEPROM.put( 0, ssid );
      while( Serial.available() )
      {
        Serial.read();
      }
      Serial.println("Enter Password:");
      i = 0;
      while(1)
      {
        if( Serial.available() )
        {
          c = Serial.read();
          if( c == '\r' || c == '\n' )
          {
            break;
          }
          password[i++] = c;
        }
      }
      Serial.println(password);
      EEPROM.put( sizeof(ssid), password );
      EEPROM.commit();
      Serial.println("Hit reset button or power cycle");
      while(1) delay(500);
    }
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  for(uint16_t i=0; i<NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }

  uint16_t index = 0;
  uint8_t ip_addr[] = {
    WiFi.localIP()[0],
    WiFi.localIP()[1],
    WiFi.localIP()[2],
    WiFi.localIP()[3]
  };
  leds[index++] = dim_green;
  for( uint8_t i=0; i<4; i++ )
  {
    for( uint8_t j=0; j<3; j++ )
    {
      uint8_t ip_base = 100;
      for( uint8_t k=0; k<j; k++ )
      {
        ip_base/=10;
      }
      uint8_t ip_digit= ( ip_addr[i] / ip_base ) % 10;
      for( uint8_t k=0; k<ip_digit; k++ )
      {
        leds[index++] = dim_white;
      }
      if( j<2 )
      {
        leds[index++] = dim_red;
      }
    }
    leds[index++] = dim_green;
  }
}

int hexAsciiToInt(char c)
{
  if (c>='0' && c<='9') return c-'0';
  else if (c>='a' && c<='f') return c-'a'+10;
}

char intToHexAscii(int hex)
{
  if (hex<10) return '0' + hex;
  else return 'a'+hex-10;
}

void ProcessInputColor(WiFiClient  cl)
{
  String favcolor="ff0000";
  int colorBegin;
  int a, r, g, b;
 
  colorBegin = HTTP_req.indexOf("/?favcolor=%");
  if (colorBegin > -1) {
    colorBegin += 12;
    a = hexAsciiToInt(HTTP_req[colorBegin+0])*16 + hexAsciiToInt(HTTP_req[colorBegin+1]);
    r = hexAsciiToInt(HTTP_req[colorBegin+2])*16 + hexAsciiToInt(HTTP_req[colorBegin+3]);
    g = hexAsciiToInt(HTTP_req[colorBegin+4])*16 + hexAsciiToInt(HTTP_req[colorBegin+5]);
    b = hexAsciiToInt(HTTP_req[colorBegin+6])*16 + hexAsciiToInt(HTTP_req[colorBegin+7]);
    favcolor = "";
    favcolor += intToHexAscii(r/16);
    favcolor += intToHexAscii(r%16);
    favcolor += intToHexAscii(g/16);
    favcolor += intToHexAscii(g%16);
    favcolor += intToHexAscii(b/16);
    favcolor += intToHexAscii(b%16);
  }

  Serial.println("favcolor="+favcolor);

  CRGB color( r, g, b );
  for(uint16_t i=0; i<NUM_LEDS; i++)
  {
    leds[i] = color;
  }
  FastLED.show();

  delay(100);
  Serial.println("ok");
 
  cl.println("<label for=\"favcolor\">Select color:</label>");
  cl.println("<input type=\"color\" id=\"favcolor\" name=\"favcolor\" value=\"#" + favcolor + "\">");
  cl.println("<input type=\"submit\" value=\"Submit\">");
}

void loop(){
  FastLED.show();
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {  // got client?
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {   // client data available to read
        char c = client.read(); // read 1 byte (character) from client
        HTTP_req += c;  // save the HTTP request 1 char at a time
        // last line of client request is blank and ends with \n
        // respond to client only after last line received
        if (c == '\n' && currentLineIsBlank) {
         // send a standard http response header
         client.println("HTTP/1.1 200 OK");
         client.println("Content-Type: text/html");
         client.println("Connection: close");
         client.println();
         // send web page
         client.println("<!DOCTYPE html>");
         client.println("<html>");
         client.println("<head>");
         client.println("<title>Arduino LED Control</title>");
         //adds styles
         client.println("<style type=\"text/css\">body {font-size:1.7rem; font-family: Georgia; text-align:center; color: #333; background-color: #cdcdcd;} div{width:75%; background-color:#fff; padding:15px; text-align:left; border-top:5px solid #bb0000; margin:25px auto;}</style>");
         client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
         client.println("</head>");
         client.println("<body>");
         client.println("<div>");
         client.println("<h1>LED color string control</h1>");
         client.println("<form method=\"get\">");
         ProcessInputColor(client);
         client.println("</form>");
         client.println("</div>");
         client.println("</body>");
         client.println("</html>");
         Serial.print(HTTP_req);
         HTTP_req = "";    // finished with request, empty string
         break;
        }
        // every line of text received from the client ends with \r\n
        if (c == '\n') {
         // last character on line of received text
         // starting new line with next character read
         currentLineIsBlank = true;
        }
        else if (c != '\r') {
         // a text character was received from client
         currentLineIsBlank = false;
        }
      } // end if (client.available())
    } // end while (client.connected())
    client.stop(); // close the connection
  } // end if (client)
}
