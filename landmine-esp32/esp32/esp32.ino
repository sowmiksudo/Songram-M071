#include <SPI.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>

// Replace with your network credentials
const char* ssid = "SongramM071";
const char* password = "PASSWORD123";

// Set web server port number to 80
WiFiServer server(80);

#define PN532_SCK (18)
#define PN532_MISO (19)
#define PN532_MOSI (23)
#define PN532_SS (27)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);


String tagId;
String header;


const int greenLedPin = 33;
const int redLedPin = 32;
const int buzzerPin = 2;
const int pushButtonPin = 4;

int pushButton = 0;
int redLed = 0;
int greenLed = 0;
String devModState = "off";

void setup() {
  pinMode(pushButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(9600);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1)
      ;  // halt
  }
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.SAMConfig();

  // start up animation

  // for (int i = 0; i < 10; i++) {
  //   digitalWrite(greenLedPin, HIGH);
  //   digitalWrite(buzzerPin, LOW);
  //   delay(300);
  //   digitalWrite(buzzerPin, HIGH);
  //   digitalWrite(greenLedPin, LOW);
  //   digitalWrite(redLedPin, HIGH);
  //   delay(300);
  //   digitalWrite(redLedPin, LOW);
  // }

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {

  WiFiClient client = server.available();  // Listen for incoming clients
  if (client) {                                                                // If a new client connects,
    Serial.println("New Client.");                                             // print a message out in the serial port
    String currentLine = "";                                                   // make a String to hold incoming data from the client
    while (client.connected()) {  // loop while the client's connected
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        Serial.write(c);         // print it out the serial monitor
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /devMod/on") >= 0) {
              Serial.println("devMod on");
              devModState = "on";
            } else if (header.indexOf("GET /devMod/off") >= 0) {
              Serial.println("DevMod Off off");
              devModState = "off";
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");

            // Display current state, and ON/OFF buttons for GPIO 26
            client.println("<p>DEVMODE - State " + devModState + "</p>");
            // If the devModState is off, it displays the ON button
            if (devModState == "off") {
              client.println("<p><a href=\"/devMod/on\"><button class=\"button\">DEVMODE ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/devMod/off\"><button class=\"button button2\">DEVMODE OFF</button></a></p>");
            }

            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else {  // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
  }

  // put your main code here, to run repeatedly:
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;
  uint16_t timeout = 1000;

  pushButton = digitalRead(pushButtonPin);  // read pushButton
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);
  tagId = "";
  // for (byte i = 0; i <= uidLength - 1; i++) {
  //   tagId += (uid[i] < 0x10 ? "0" : "") + String(uid[i], HEX);
  // }
  // Serial.println(tagId);


  // Serial.println("Button reading");

  if (!success) {
    if (pushButton == HIGH) {
      digitalWrite(buzzerPin, LOW);
      digitalWrite(redLedPin, LOW);
      // Serial.println("Push button high");
    } else {
      digitalWrite(buzzerPin, HIGH);
      digitalWrite(redLedPin, HIGH);
      //  Serial.println("Push button low");
    }
  } else {
    Serial.println("[o] Detected NFC Scan!!");
    // if (tagId == "66 74 15 1F" || tagId == "8E 0C 42 83") {
    if (pushButton == HIGH) {
      digitalWrite(greenLedPin, LOW);
    } else {
      digitalWrite(greenLedPin, HIGH);
    }
    // }
  }

  // Clear the header variable
  header = "";
  // Close the connection
  client.stop();
  Serial.println("Client disconnected.");
  Serial.println("");
}