// for SPI Communication
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <NfcAdapter.h>
PN532_SPI interface(SPI, 10);            // create a PN532 SPI interface with the SPI CS terminal located at digital pin 10
NfcAdapter nfc = NfcAdapter(interface);  // create an NFC adapter object
String tagId = "None";

const int greenLedPin = 5;
const int redLedPin = 3;

int pushButton = 0;
int redLed = 0;
int greenLed = 0;
void setup(void) {
  pinMode(2, INPUT_PULLUP);
  pinMode(7, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("System initialized");
  nfc.begin();
}

void loop() {

  pushButton = digitalRead(2);  // read pushButton

  if (!nfc.tagPresent()) {
    if (pushButton == HIGH) {
      digitalWrite(7, LOW);
      // Serial.println("GREEN LIGHT ACTIVE");
      digitalWrite(redLedPin, LOW);
    } else {
      digitalWrite(7, HIGH);
      // delay(2000);
      digitalWrite(redLedPin, HIGH);
    }
  } else {
    // digitalWrite()
    NfcTag tag = nfc.read();
    Serial.println("[o] Detected NFC Scan!!");
    //  tag.print();
    tagId = tag.getUidString();
    Serial.println(tagId);
    // if (tagId == "8E 0C 42 83") {
    //   Serial.println("NFC Ring Detected!");
    // }
    if (tagId == "66 74 15 1F" || tagId == "8E 0C 42 83") {
      Serial.println("NFC Card Detected!");
      if (pushButton == HIGH) {
        digitalWrite(greenLedPin, LOW);
        // Serial.println("GREEN LIGHT ACTIVE");
      } else {
        digitalWrite(greenLedPin, HIGH);
      }
    }
  }
}
