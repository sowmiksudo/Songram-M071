#include <SPI.h>
#include <Adafruit_PN532.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <CuteBuzzerSounds.h>
#include <TinyGPS++.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>



#define PN532_SCK (18)
#define PN532_MISO (19)
#define PN532_MOSI (23)
#define PN532_SS (27)

#define RXD2 35
#define TXD2 34

/************************* WiFi Access Point *********************************/

#define WLAN_SSID "Tenda"  // can't be longer than 32 characters!
#define WLAN_PASS "1234567890"

#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "sowmik"
#define AIO_KEY "aio_OUMR98844pbCe9iIRa2DQxZHZ6TX"

const char* ssid = "Esp32WiFi";
const char* password = "esp32222";
const char* mqttServer = "io.adafruit.com";
const int mqttPort = 1883;
const char* mqttUser = "sowmik";
const char* mqttPassword = "aio_OUMR98844pbCe9iIRa2DQxZHZ6TX";
const char* mqttTopic = AIO_USERNAME "/feeds/songram_activation";
const char* locTopic = AIO_USERNAME "/feeds/m071_location/csv";

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

WiFiClient espWifi;
PubSubClient client(espWifi);

HTTPClient http;

HardwareSerial neogps(1);
TinyGPSPlus gps;

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
String bt = "82";
String HOME_LAT = "23.776924287083087";
String HOME_LNG = "90.3907966483282";
// String HOME_LAT = "23.75353838231597"; HOME
// String HOME_LNG = "90.38740482735874";

String LOCATION_STR = "0," + HOME_LAT + "," + HOME_LNG + "," + "6";
String LAT_LNG = HOME_LAT + "," + HOME_LNG;

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
  Serial2.begin(9600);
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
  http.begin("https://api.weatherapi.com/v1/current.json?key=8b281a65e8894d57ac3113033232611&aqi=no&q=" + LAT_LNG);
  int httpCode = http.GET();
  String payload = http.getString();
  
  Serial.println(payload);

  StaticJsonDocument<1536> doc;

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  JsonObject current = doc["current"];
  long current_last_updated_epoch = current["last_updated_epoch"];  // 1701006300
  const char* current_last_updated = current["last_updated"];       // "2023-11-26 19:45"
  float current_temp_c = current["temp_c"];                         // 24.8
  float current_temp_f = current["temp_f"];                         // 76.7
  int current_is_day = current["is_day"];                           // 0

  JsonObject current_condition = current["condition"];
  const char* current_condition_text = current_condition["text"];  // "Clear"
  const char* current_condition_icon = current_condition["icon"];
  int current_condition_code = current_condition["code"];  // 1000

  float current_wind_mph = current["wind_mph"];        // 4.3
  float current_wind_kph = current["wind_kph"];        // 6.8
  int current_wind_degree = current["wind_degree"];    // 86
  const char* current_wind_dir = current["wind_dir"];  // "E"
  int current_pressure_mb = current["pressure_mb"];    // 1014
  float current_pressure_in = current["pressure_in"];  // 29.93
  int current_precip_mm = current["precip_mm"];        // 0
  int current_precip_in = current["precip_in"];        // 0
  int current_humidity = current["humidity"];          // 52
  int current_cloud = current["cloud"];                // 0
  float current_feelslike_c = current["feelslike_c"];  // 25.8
  float current_feelslike_f = current["feelslike_f"];  // 78.5
  int current_vis_km = current["vis_km"];              // 10
  int current_vis_miles = current["vis_miles"];        // 6
  int current_uv = current["uv"];                      // 1
  float current_gust_mph = current["gust_mph"];        // 8.4
  float current_gust_kph = current["gust_kph"];        // 13.5

  String wind_dir = String(current_wind_degree) + "° " +current_wind_dir;
  String weather_data = "\n";
  weather_data += "Temperature  ::  ";
  weather_data += String(current_temp_c, 1);
  weather_data += "° C\n";
  weather_data += "=> Wind Speed   ::  ";
  weather_data += String(current_wind_mph, 2);
  weather_data += " mph\n";
  weather_data += "=> Wind Angle   ::  ";
  weather_data += wind_dir;
  weather_data += "\n";
  weather_data += "=> Humidity     ::  ";
  weather_data += current_humidity;
  weather_data += "%\n";
  weather_data += "=> UV index     ::  ";
  weather_data += current_uv;
  weather_data += "\n";
  weather_data += "=> Air Pressure ::  ";
  weather_data += current_pressure_mb;
  weather_data += " mm(Hg)";


  Serial.println(weather_data);
  client.publish("sowmik/feeds/m071_weather", weather_data.c_str());

  client.subscribe(mqttTopic);
  client.publish("sowmik/feeds/m071_battery", bt.c_str());
  client.publish("sowmik/feeds/m071_location/csv", LOCATION_STR.c_str());
  String message = "[X] SONGRAM (M071_32874) IS NOW CONNECTED TO THE NETWORK!";
  client.publish("sowmik/feeds/m071_np532", message.c_str());
  Serial.println(LOCATION_STR);
  cute.play(S_CONNECTION);
}

uint32_t x = 0;

void loop() {

  if (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid()) {
        Serial.print(F("- latitude: "));
        Serial.println(gps.location.lat());

        Serial.print(F("- longitude: "));
        Serial.println(gps.location.lng());

        // Serial.print(F("- altitude: "));
        // if (gps.altitude.isValid())
        //   Serial.println(gps.altitude.meters());
        // else
        //   Serial.println(F("INVALID"));
      } else {
        Serial.println(F("- location: INVALID"));
      }
    }
  }

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
  if (isOn) {
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
