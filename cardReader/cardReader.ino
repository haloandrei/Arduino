#include <SPI.h>
#include <Adafruit_PN532.h>

// ===== ESP32 SPI pins (VSPI) =====
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23
#define PIN_SPI_SS     5

Adafruit_PN532 nfc(PIN_SPI_SS);

// Helper: pretty-print UID
void printUID(uint8_t *uid, uint8_t uidLength) {
  Serial.print("UID: ");
  for (uint8_t i = 0; i < uidLength; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
    if (i < uidLength - 1) Serial.print(":");
  }
  Serial.println();
}

// Helper: guess card type from UID length
void printCardType(uint8_t uidLength) {
  Serial.print("Detected ISO14443A tag with ");
  Serial.print(uidLength);
  Serial.println("-byte UID.");

  if (uidLength == 4) {
    Serial.println("→ Likely: MIFARE Classic (1K / 4K) or MIFARE Mini.");
    Serial.println("  These are the common access control / building entry cards.");
  } else if (uidLength == 7) {
    Serial.println("→ Likely: MIFARE Ultralight / NTAG / DESFire / other 7-byte UID tag.");
    Serial.println("  Often used for NFC stickers, tags, some transport cards, etc.");
  } else if (uidLength == 10) {
    Serial.println("→ 10-byte UID tags are rarer; check documentation of the card.");
  } else {
    Serial.println("→ Unknown / uncommon UID length. Check the chip marking for exact type.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize SPI for ESP32 VSPI
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);

  Serial.println("Initializing PN532...");
  nfc.begin();

  uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println("ERROR: PN532 not found. Check wiring and power (3.3V)!");
    while (1) delay(10);
  }

  Serial.print("Found PN532, firmware: 0x");
  Serial.println(version, HEX);

  // Configure PN532 to read ISO14443A cards
  nfc.SAMConfig();

  Serial.println("PN532 ready. Bring a card close to the reader...");
}

void loop() {
  uint8_t uid[7];      // Buffer to store UID
  uint8_t uidLength;   // UID length (4 or 7 bytes typically)

  // Try to read a passive target (ISO14443A)
  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("\n--- CARD DETECTED ---");
    printUID(uid, uidLength);
    printCardType(uidLength);
    Serial.println("----------------------");

    delay(1000); // small delay so it doesn't spam when card stays on reader
  }
}
