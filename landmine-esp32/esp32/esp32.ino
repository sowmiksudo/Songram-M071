#include <SPI.h>
#include <Adafruit_PN532.h>
#define PN532_SCK (18)
#define PN532_MISO (19)
#define PN532_MOSI (23)
#define PN532_SS (27)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

String idcard;

void setup() {
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
}

void loop() {
  // put your main code here, to run repeatedly:
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    idcard = "";
    for (byte i = 0; i <= uidLength - 1; i++) {
      idcard += (uid[i] < 0x10 ? "0" : "") + String(uid[i], HEX);
    }
    Serial.print("ID CARD : ");
    Serial.print(idcard);
    Serial.println("");
  }
}