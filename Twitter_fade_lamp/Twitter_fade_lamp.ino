#include <SPI.h>
#include <WiFi.h>
#include "structRgb.h"
#include <avr/pgmspace.h>

char *queryString1 = "#Champions";
char *queryString2 = "#arduino";

char ssid[] = "OFFICINE ARDUINO"; //  your network SSID (name) 
char pass[] = "T3mporary!";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS; // status of the wifi connection


const int pin_r = 5;
const int pin_g = 6;
const int pin_b = 3;

const uint8_t mode1_switch = 2;
const uint8_t mode2_switch = 8;
uint8_t prev_mode1Switch = true;
uint8_t prev_mode2Switch = true;

const int maxTweets = 5; // Limit tweets printed; avoid runaway output

const unsigned long // Time limits, expressed in milliseconds:
pollingInterval = 10L * 1000L, // Note: Twitter server will allow 150/hr max
connectTimeout  = 15L * 1000L, // Max time to retry server link
responseTimeout = 15L * 1000L, // Max time to wait for data from server
fadeInterval = 1000;

unsigned long prevMillisFade = 0;

byte sleepPos = 0; // Current "sleep throb" table position
byte resultsDepth; // Used in JSON parsing

uint8_t colorCount = 0;
uint8_t prev_colorCount = 0;
const int maxColorCount = 8;


rgb lampShades[maxColorCount];


WiFiClient client;
char *serverName = "search.twitter.com",
// queryString can be any valid Twitter API search string, including
// boolean operators. See https://dev.twitter.com/docs/using-search
// for options and syntax. Funny characters do NOT need to be URL
// encoded here -- the sketch takes care of that.
queryString[20], 
lastId[21], // 18446744073709551615\0 (64-bit maxint as string)
timeStamp[32], // WWW, DD MMM YYYY HH:MM:SS +XXXX\0
fromUser[16], // Max username length (15) + \0
msgText[141], // Max tweet length (140) + \0
name[11], // Temp space for name:value parsing
value[141]; // Temp space for name:value parsing


// Function prototypes -------------------------------------------------------

boolean
jsonParse(int, byte),
readString(char *, int);
int
unidecode(byte),
timedRead(void);
void
pippo(rgb, rgb);

// ---------------------------------------------------------------------------

void setup() {
  pinMode(mode1_switch, INPUT_PULLUP);
  pinMode(mode2_switch, INPUT_PULLUP);
  
  
  analogWrite(pin_r, 255);
  analogWrite(pin_g, 0);
  analogWrite(pin_b, 0);
  delay(1000);
  
  analogWrite(pin_r, 0);
  analogWrite(pin_g, 255);
  analogWrite(pin_b, 0);
  delay(1000);
  
  analogWrite(pin_r, 0);
  analogWrite(pin_g, 0);
  analogWrite(pin_b, 255);
  delay(1000);

  analogWrite(pin_r, 0);
  analogWrite(pin_g, 0);
  analogWrite(pin_b, 0);

  Serial.begin(9600);
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present")); 
    // don't continue:
    while(true);
  } 

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) { 
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    status = WiFi.begin(ssid, pass); 

    // wait 10 seconds for connection:
    delay(5000);
  } 
  // you're connected now, so print out the status:
  printWifiStatus();

  // Clear all string data
  memset(lastId , 0, sizeof(lastId));
  memset(timeStamp, 0, sizeof(timeStamp));
  memset(fromUser , 0, sizeof(fromUser));
  memset(msgText , 0, sizeof(msgText));
  memset(name , 0, sizeof(name));
  memset(value , 0, sizeof(value));
}

// ---------------------------------------------------------------------------

void loop() {
  unsigned long startTime, t;
  int i;
  char c;

  startTime = millis();
   
  uint8_t mode1_switchStatus = digitalRead(mode1_switch);
  uint8_t mode2_switchStatus = digitalRead(mode2_switch);
  
  if(mode1_switchStatus == false && prev_mode1Switch == true) {
    Serial.println("ARDUINO");
    colorCount = 0;
    strcpy(queryString, queryString1);
    for(uint8_t i=0; i<8; i++) {
      lampShades[i].red = 255;
      lampShades[i].green = i*15;
      lampShades[i].blue = 0;
      
      Serial.print(i);
      Serial.print(": ");
      Serial.print(lampShades[i].red);
      Serial.print(" ");
      Serial.print(lampShades[i].green);
      Serial.print(" ");
      Serial.println(lampShades[i].blue);
     
    }
      analogWrite(pin_r, lampShades[0].red);
      analogWrite(pin_g, lampShades[0].green);
      analogWrite(pin_b, lampShades[0].blue);
  }
  else if(mode2_switchStatus == false && prev_mode2Switch == true) {
    Serial.println("CIAO");
    colorCount = 0;
    strcpy(queryString, queryString2);
    for(uint8_t i=0; i<8; i++) {
      lampShades[i].red = 0;
      lampShades[i].green = i*15;
      lampShades[i].blue = 255;
      
      Serial.print(i);
      Serial.print(": ");
      Serial.print(lampShades[i].red);
      Serial.print(" ");
      Serial.print(lampShades[i].green);
      Serial.print(" ");
      Serial.println(lampShades[i].blue);

    }
          
      analogWrite(pin_r, lampShades[0].red);
      analogWrite(pin_g, lampShades[0].green);
      analogWrite(pin_b, lampShades[0].blue);
  }
  prev_mode1Switch = mode1_switchStatus;
  prev_mode2Switch = mode2_switchStatus;
  
  Serial.print(prev_mode1Switch);
  Serial.print(" ");
  Serial.println(prev_mode2Switch);
  
  Serial.println(queryString);
  Serial.println();
  
  // Attempt server connection, with timeout...
  Serial.print(F("Connecting to server..."));
  while((client.connect(serverName, 80) == false) &&
    ((millis() - startTime) < connectTimeout));

  if(client.connected()) { // Success!
    Serial.print(F("OK\r\nIssuing HTTP request..."));
    // URL-encode queryString to client stream:
    client.print(F("GET /search.json?q="));
    for(i=0; c=queryString[i]; i++) {
      if(((c >= 'a') && (c <= 'z')) ||
        ((c >= 'A') && (c <= 'Z')) ||
        ((c >= '0') && (c <= '9')) ||
        (c == '-') || (c == '_') ||
        (c == '.') || (c == '~')) {
        // Unreserved char: output directly
        client.write(c);
      } 
      else {
        // Reserved/other: percent encode
        client.write('%');
        client.print(c, HEX);
      }
    }
    client.print(F("&result_type=recent&include_entities=false&rpp="));
    if(lastId[0]) {
      client.print(maxTweets); // Limit to avoid runaway printing
      client.print(F("&since_id=")); // Display tweets since prior query
      client.print(lastId);
    } 
    else {
      client.print('1'); // First run; show single latest tweet
    }
    client.print(F(" HTTP/1.1\r\nHost: "));
    client.println(serverName);
    client.println(F("Connection: close\r\n"));

    Serial.print(F("OK\r\nAwaiting results (if any)..."));
    t = millis();
    while((!client.available()) && ((millis() - t) < responseTimeout));
    if(client.available()) { // Response received?
      // Could add HTTP response header parsing here (400, etc.)
      if(client.find("\r\n\r\n")) { // Skip HTTP response header
        Serial.println(F("OK\r\nProcessing results..."));
        resultsDepth = 0;
        jsonParse(0, 0);
      } 
      else Serial.println(F("response not recognized."));
    } 
    else Serial.println(F("connection timed out."));
    client.stop();
  } 
  else { // Couldn't contact server
    Serial.println(F("failed"));
  }
  

  // Sometimes network access & printing occurrs so quickly, the steady-on
  // LED wouldn't even be apparent, instead resembling a discontinuity in
  // the otherwise smooth sleep throb. Keep it on at least 4 seconds.
  t = millis() - startTime;
  if(t < 4000L) delay(4000L - t);

  // Pause between queries, factoring in time already spent on network
  // access, parsing, printing and LED pause above.
  t = millis() - startTime;
  if(t < pollingInterval) {
    Serial.print(F("Pausing..."));
    delay(pollingInterval - t);
    Serial.println(F("done"));
  }
}

// ---------------------------------------------------------------------------

boolean jsonParse(int depth, byte endChar) {
  int c, i;
  boolean readName = true;

  for(;;) {
    while(isspace(c = timedRead())); // Scan past whitespace
    if(c < 0) return false; // Timeout
    if(c == endChar) return true; // EOD

    if(c == '{') { // Object follows
      if(!jsonParse(depth + 1, '}')) return false;
      if(!depth) return true; // End of file
      if(depth == resultsDepth) { // End of object in results list

        // Dump to serial console as well
        Serial.print(F("User: "));
        Serial.println(fromUser);
        Serial.print(F("Text: "));
        Serial.println(msgText);
        Serial.print(F("Time: "));
        Serial.println(timeStamp);

        // Clear strings for next object
        timeStamp[0] = fromUser[0] = msgText[0] = 0;
      }
    } 
    else if(c == '[') { // Array follows
      if((!resultsDepth) && (!strcasecmp(name, "results")))
        resultsDepth = depth + 1;
      if(!jsonParse(depth + 1,']')) return false;
    } 
    else if(c == '"') { // String follows
      if(readName) { // Name-reading mode
        if(!readString(name, sizeof(name)-1)) return false;
      } 
      else { // Value-reading mode
        if(!readString(value, sizeof(value)-1)) return false;
        // Process name and value strings:
        if (!strcasecmp(name, "max_id_str")) {
          strncpy(lastId, value, sizeof(lastId)-1);
        } 
        else if(!strcasecmp(name, "created_at")) {
          strncpy(timeStamp, value, sizeof(timeStamp)-1);
        } 
        else if(!strcasecmp(name, "from_user")) {
          strncpy(fromUser, value, sizeof(fromUser)-1);
/*
	  colorCount++;
	  if(colorCount == maxColorCount)
	    colorCount = 0;
	  */
	  Serial.println(colorCount);
	  
	  uint8_t cc = (colorCount+1)%8;
	  
	  pippo(lampShades[colorCount], lampShades[cc]);
	  
	  colorCount = cc;
	  
	  delay(5000);

	}
        else if(!strcasecmp(name, "text")) {
          strncpy(msgText, value, sizeof(msgText)-1);
        }
      }
    } 
    else if(c == ':') { // Separator between name:value
      readName = false; // Now in value-reading mode
      value[0] = 0; // Clear existing value data
    } 
    else if(c == ',') {
      // Separator between name:value pairs.
      readName = true; // Now in name-reading mode
      name[0] = 0; // Clear existing name data
    } // Else true/false/null or a number follows. These values aren't
    // used or expected by this program, so just ignore...either a comma
    // or endChar will come along eventually, these are handled above.
  }
}

// ---------------------------------------------------------------------------

// Read string from client stream into destination buffer, up to a maximum
// requested length. Buffer should be at least 1 byte larger than this to
// accommodate NUL terminator. Opening quote is assumed already read,
// closing quote will be discarded, and stream will be positioned
// immediately following the closing quote (regardless whether max length
// is reached -- excess chars are discarded). Returns true on success
// (including zero-length string), false on timeout/read error.
boolean readString(char *dest, int maxLen) {
  int c, len = 0;

  while((c = timedRead()) != '\"') { // Read until closing quote
    if(c == '\\') { // Escaped char follows
      c = timedRead(); // Read it
      // Certain escaped values are for cursor control --
      // there might be more suitable printer codes for each.
      if (c == 'b') c = '\b'; // Backspace
      else if(c == 'f') c = '\f'; // Form feed
      else if(c == 'n') c = '\n'; // Newline
      else if(c == 'r') c = '\r'; // Carriage return
      else if(c == 't') c = '\t'; // Tab
      else if(c == 'u') c = unidecode(4);
      else if(c == 'U') c = unidecode(8);
      // else c is unaltered -- an escaped char such as \ or "
    } // else c is a normal unescaped char

    if(c < 0) return false; // Timeout

    // In order to properly position the client stream at the end of
    // the string, characters are read to the end quote, even if the max
    // string length is reached...the extra chars are simply discarded.
    if(len < maxLen) dest[len++] = c;
  }

  dest[len] = 0;
  return true; // Success (even if empty string)
}

// ---------------------------------------------------------------------------

// Read a given number of hexadecimal characters from client stream,
// representing a Unicode symbol. Return -1 on error, else return nearest
// equivalent glyph in printer's charset. (See notes below -- for now,
// always returns '-' or -1.)
int unidecode(byte len) {
  int c, v, result = 0;
  while(len--) {
    if((c = timedRead()) < 0) return -1; // Stream timeout
    if ((c >= '0') && (c <= '9')) v = c - '0';
    else if((c >= 'A') && (c <= 'F')) v = 10 + c - 'A';
    else if((c >= 'a') && (c <= 'f')) v = 10 + c - 'a';
    else return '-'; // garbage
    result = (result << 4) | v;
  }

  // To do: some Unicode symbols may have equivalents in the printer's
  // native character set. Remap any such result values to corresponding
  // printer codes. Until then, all Unicode symbols are returned as '-'.
  // (This function still serves an interim purpose in skipping a given
  // number of hex chars while watching for timeouts or malformed input.)

  return '-';
}

// ---------------------------------------------------------------------------

// Read from client stream with a 5 second timeout. Although an
// essentially identical method already exists in the Stream() class,
// it's declared private there...so this is a local copy.
int timedRead(void) {
  unsigned long start = millis();

  while((!client.available()) && ((millis() - start) < 5000L));

  return client.read(); // -1 on timeout
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());
  
  //Serial.println(freeRam());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print(F("signal strength (RSSI):"));
  Serial.print(rssi);
  Serial.println(F(" dBm"));
}

void pippo(rgb in, rgb out) {
  boolean stop_r, stop_g, stop_b;
  stop_r = stop_g = stop_b = false;
  
  Serial.print(in.red);
  Serial.print(" ");
  Serial.print(in.green);
  Serial.print(" ");
  Serial.print(in.blue);

  Serial.print(" - ");
  
  Serial.print(out.red);
  Serial.print(" ");
  Serial.print(out.green);
  Serial.print(" ");
  Serial.println(out.blue);
  
  while(!(stop_r & stop_g & stop_b)) {
    // operate each color: RED
   
    if(in.red > 0 && in.red < out.red && out.red <= 255) {
      stop_r = false;
      in.red++;
    }
    if(in.red <= 255 && in.red >out.red && out.red > 0){
      in.red--;
      stop_r = true;
    }

    if(in.green < out.green){
      stop_g = false;
      in.green++;
    }
    if(in.green >= out.green) {
      in.green=out.green;
      stop_g = true;
    }
    
    if(in.blue < out.blue){
      stop_b = false;
      in.blue++;
    }
    if(in.blue >= out.blue) {
      in.blue=out.blue;
      stop_b = true;
    }

       
    Serial.print(stop_r);
    Serial.print(stop_g);
    Serial.println(stop_b);
    Serial.println((stop_r | stop_g | stop_b));
    // use for debugging purposes, this is
    // very noisy otherwise ... too much info
    
    Serial.print("R: ");
    Serial.print(in.red);
    Serial.print(" - G: ");
    Serial.print(in.green);
    Serial.print(" - B: ");
    Serial.println(in.blue);
    

    // push out the colors
    analogWrite(pin_r, in.red);
    analogWrite(pin_g, in.green);
    analogWrite(pin_b, in.blue);
    
    delay(100);
  }
  
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}