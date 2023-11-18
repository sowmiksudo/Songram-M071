#include <SPI.h>
#include <Adafruit_PN532.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFi.h>
#include <WiFiClient.h>


#define PN532_SCK (18)
#define PN532_MISO (19)
#define PN532_MOSI (23)
#define PN532_SS (27)

/************************* WiFi Access Point *********************************/

#define WLAN_SSID "Tenda"  // can't be longer than 32 characters!
#define WLAN_PASS "1234567890"

#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "sowmik"
#define AIO_KEY "aio_OUMR98844pbCe9iIRa2DQxZHZ6TX"

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);


WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/songram_activation");
const char TEXT_LOGS[] PROGMEM = AIO_USERNAME "/feeds/m071_np532";
Adafruit_MQTT_Publish text_logs = Adafruit_MQTT_Publish(&mqtt, TEXT_LOGS);


const String SONGRAM_ID = "M321879";
String tagId;
bool isOn = true; 
const int greenLedPin = 33;
const int redLedPin = 32;
const int buzzerPin = 2;
const int pushButtonPin = 4;

float lat = "23.75353838231597";
float lng = "90.38740482735874";
// {"value": 0, "lat": "23.75353838231597", "lon": "90.38740482735874"}
int pushButton = 0;
int redLed = 0;
int greenLed = 0;


void MQTT_connect();

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


  Serial.println(F("Adafruit MQTT Connection"));

  // Connect to WiFi access point.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(3000);
  // Setup MQTT subscription for onoff & slider feed.
  mqtt.subscribe(&onoffbutton);
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
}

uint32_t x = 0;

void loop() {
  //// Wifi

  MQTT_connect();  //

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    // Check if its the onoff button feed
    if (subscription == &onoffbutton) {
      Serial.print(F("On-Off button: "));
      Serial.println((char *)onoffbutton.lastread);
      if (strcmp((char *)onoffbutton.lastread, "true") == 0) {
        isOn = true;
        text_logs.publish("[X] Songram (M071_321879) is now active!!");
      }
      if (strcmp((char *)onoffbutton.lastread, "false") == 0) {
        isOn = false;
        text_logs.publish("[X] Songram (M071_321879) has been set to maintanance mode.");
      }
    }
  }

  // -- Reading NFC RFID Tags and doing the main process -- 

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

  // Don't know why but reading rfid tag stops the rest of the code !!

  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, LOW);
  if (isOn == true) {
    if (!success) {
      if (pushButton == HIGH) {
        digitalWrite(buzzerPin, LOW);
        digitalWrite(redLedPin, LOW);
      } else {
        digitalWrite(buzzerPin, HIGH);
        digitalWrite(redLedPin, HIGH);
      }
    } else {
      Serial.println("[o] Detected NFC Scan!!");
      text_logs.publish("[o] Detected NFC Scan!!");
      if (pushButton == HIGH) {
        digitalWrite(greenLedPin, LOW);
      } else {
        digitalWrite(greenLedPin, HIGH);
      }
      // }
    }
  } else {
    digitalWrite(greenLedPin, HIGH);
  }


  // ping the server to keep the mqtt connection alive
  if (!mqtt.ping()) {
    mqtt.disconnect();
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {  // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection...");
    mqtt.disconnect();
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1)
        ;
    }
  }
  Serial.println("MQTT Connected!");
}
