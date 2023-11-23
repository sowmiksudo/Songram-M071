#include <SPI.h>
#include <Adafruit_PN532.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <CuteBuzzerSounds.h>


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

const char* ssid = "Tenda";
const char* password = "1234567890";
const char* mqttServer = "io.adafruit.com";
const int mqttPort = 1883;
const char* mqttUser = "sowmik";
const char* mqttPassword = "aio_OUMR98844pbCe9iIRa2DQxZHZ6TX";
const char* mqttTopic = AIO_USERNAME "/feeds/songram_activation";
const char* locTopic = AIO_USERNAME "/feeds/m071_location/csv";

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

WiFiClient espWifi;
PubSubClient client(espWifi);

const String SONGRAM_ID = "M321879";
String tagId;
String activationState;
bool isOn = true;
const int greenLedPin = 33;
const int redLedPin = 32;
const int buzzerPin = 2;
const int pushButtonPin = 4;

int pushButton = 0;
int redLed = 0;
int greenLed = 0;
// {"value": 0, "lat": "23.75353838231597", "lon": "90.38740482735874"}
String bt = "67";
String HOME_LAT = "23.75353838231597";
String HOME_LNG = "90.38740482735874";
String LOCATION_STR = "0," + HOME_LAT + "," + HOME_LNG +"," + "6";

void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Callback function
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  activationState = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    activationState += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "sowmik/feeds/songram_activation") {
    Serial.print("Changing output to " + activationState);
    String message;
    if (activationState == "true") {
      isOn = true;
      message = "[O] SONGRAM (M071_32874) IS NOW ACTIVE.";
      client.publish("sowmik/feeds/m071_np532", message.c_str());
    } else if (activationState == "false") {
      isOn = false;
      message = "[X] SONGRAM (M071_32874) IS NOW DISABLED.";
      client.publish("sowmik/feeds/m071_np532", message.c_str());
    }
  }
}


void setup_nfc() {
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
}

void setup() {
  pinMode(pushButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  cute.init(buzzerPin);
  cute.play(S_SURPRISE);

  Serial.begin(9600);
  setup_nfc();
  delay(200);
  setup_wifi();
  delay(200);
  // Setup MQTT subscription for onoff feed.

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  Serial.println("Connecting to MQTT…");
  while (!client.connected()) {
    String clientId = "Songram-Client";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state  ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  client.subscribe(mqttTopic);
  client.publish("sowmik/feeds/m071_battery", bt.c_str());
  client.publish("sowmik/feeds/m071_location/csv", LOCATION_STR.c_str());
  Serial.println(LOCATION_STR);
  cute.play(S_CONNECTION);
}

uint32_t x = 0;

void loop() {
  String msg;
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // if (isOn) {
  //   msg = "[x] Songram is now ACTIVE!";
  //   Serial.println(msg);
  // } else {
  //   msg = "[x] Songram is now DISABLED.";
  //   Serial.println(msg);
  // }



  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;
  uint16_t timeout = 1000;

  pushButton = digitalRead(pushButtonPin);  // read pushButton
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);
  tagId = "";
  for (byte i = 0; i <= uidLength - 1; i++) {
    tagId += (uid[i] < 0x10 ? "0" : "") + String(uid[i], HEX);
  }
  // Serial.println(tagId);

  // Don't know why but reading rfid tag stops the rest of the code !!

  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, LOW);
  if(isOn){
  if (!success) {
    if (pushButton == HIGH) {
      digitalWrite(redLedPin, LOW);
    } else {
      digitalWrite(redLedPin, HIGH);
      msg = "[X] AN ENEMY HAS PRESSED THE MINE! SONGRAM ID :: M071_32874";
      client.publish("sowmik/feeds/m071_np532", msg.c_str());
      for (int i = 0; i < 3; i++) {
        cute.play(S_FART1);
        cute.play(S_FART2);
      }
    }
  } else {
    Serial.println("[o] Detected NFC Scan!!");
    if (pushButton == HIGH) {
      digitalWrite(greenLedPin, LOW);
    } else {
      digitalWrite(greenLedPin, HIGH);
      msg = "[X] OUR SOLDIER HAS PRESSED THE MINE! SONGRAM ID:: M071_32874 | SOLDIER ID :: " + tagId;
      client.publish("sowmik/feeds/m071_np532", msg.c_str());
      delay(3000);
    }
  }
  } else {
    delay(300);
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, HIGH);
  }
}


// MQTT reconnect

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Songram-Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
