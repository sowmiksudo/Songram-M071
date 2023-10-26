#include <SPI.h>
#include <Adafruit_PN532.h>
#define PN532_SCK (18)
#define PN532_MISO (19)
#define PN532_MOSI (23)
#define PN532_SS (27)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);


String tagId;


const int greenLedPin = 33;
const int redLedPin = 32;
const int buzzerPin = 2;
const int pushButtonPin = 4;

int pushButton = 0;
int redLed = 0;
int greenLed = 0;

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

  for(int i = 0; i < 10; i++){
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(buzzerPin, LOW);
    delay(300);
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
    delay(300);
    digitalWrite(redLedPin, LOW);
  }

}

void loop() {
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
}