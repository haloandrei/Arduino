/* PN532 - write NDEF URI example (refactored to avoid block 0)
   - Supports MIFARE Classic 1K (writes 16-byte blocks starting at block 4)
   - Supports NTAG / Type 2 tags (writes 4-byte pages starting at page 4)
   Uses Adafruit_PN532 library.
*/

#include <SPI.h>
#include <Adafruit_PN532.h>

// SPI pins (adjust to your wiring)
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_MISO (19)
#define PN532_SS   (5)
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Default MIFARE Classic key A
uint8_t defaultKeyA[6] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };

// Build NDEF TLV for "https://example.com"
// TLV: 0x03, length (0x10) then NDEF record (16 bytes), then terminator 0xFE
// NDEF short record for a URI using prefix code 0x03 ("https://")
const uint8_t ndef_full[] = {
  0x03, 0x10,             // NDEF TLV tag, length = 16 (NDEF record size)
  // NDEF record (16 bytes)
  0xD1,                   // MB=1, ME=1, SR=1, TNF=0x1 (well-known)
  0x01,                   // TYPE LENGTH = 1 ('U')
  0x0C,                   // PAYLOAD LENGTH = 12 (1 identifier + 11 chars "example.com")
  0x55,                   // TYPE = 'U' (URI Record)
  0x03,                   // URI identifier code 0x03 => "https://"
  // "example.com" ASCII (11 bytes)
  0x65,0x78,0x61,0x6D,0x70,0x6C,0x65,0x2E,0x63,0x6F,0x6D
  // that's 16 bytes total for the NDEF record above
};
// The TLV + record above is 18 bytes; we append terminator 0xFE when writing
// Full payload to write = ndef_full (18 bytes) + 0xFE (terminator) => 19 bytes total

void setup() {
  Serial.begin(115200);
  Serial.println("PN532 NDEF write example (avoids block 0).");

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN532. Check wiring.");
    while (1) delay(10);
  }
  nfc.SAMConfig();
  Serial.println("PN532 initialized. Tap a tag to write NDEF (https://example.com).");
}

void loop() {
  boolean success;
  uint8_t uid[7];
  uint8_t uidLen;

  // Wait for an ISO14443A tag
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 1000);
  if (!success) {
    delay(200);
    return;
  }

  Serial.print("Tag detected, UID: ");
  for (uint8_t i = 0; i < uidLen; i++) {
    Serial.print(uid[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Check if tag is MIFARE Classic (library gives no perfect type flag from readPassiveTargetID)
  // We'll try Classic auth on block 4: if auth succeeds we assume Classic; otherwise attempt NTAG writes.
  const uint8_t testBlock = 4; // safe: not block 0
  bool isClassic = false;

  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLen, testBlock, 0 /*Key A*/, defaultKeyA)) {
    Serial.println("MIFARE Classic detected (auth succeeded on block 4). Proceeding to write MIFARE Classic blocks.");
    isClassic = true;
  } else {
    Serial.println("Auth failed for block 4 => not MIFARE Classic (or non-default key). Trying NTAG/Type2 path.");
  }

  if (isClassic) {
    // Prepare the two 16-byte blocks we will write:
    // - Block 4: first 16 bytes of (TLV + NDEF record)
    // - Block 5: remaining 2 bytes of TLV+record, then 0xFE terminator, then padding zeros to 16 bytes
    uint8_t block4[16];
    uint8_t block5[16];

    // compose the 18-byte TLV+record into a buffer
    uint8_t tmp[19]; // 18 bytes TLV+record + 0xFE
    memcpy(tmp, ndef_full, sizeof(ndef_full));       // first 18 bytes
    tmp[18] = 0xFE;                                  // terminator

    // fill block 4 = tmp[0..15]
    for (int i = 0; i < 16; ++i) block4[i] = tmp[i];

    // block5 = tmp[16..18] then pad with 0x00
    for (int i = 0; i < 16; ++i) {
      if (i < 3) block5[i] = tmp[16 + i]; // tmp[16], tmp[17], tmp[18]
      else block5[i] = 0x00;
    }

    // Authenticate block 4 again just before write (redundant but safe)
    if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLen, testBlock, 0 /*Key A*/, defaultKeyA)) {
      Serial.println("Auth failed (unexpected). Aborting writes.");
      delay(1000);
      return;
    }

    // Write block 4
    if (!nfc.mifareclassic_WriteDataBlock(testBlock, block4)) {
      Serial.println("Write failed for block 4.");
      delay(1000);
      return;
    }
    Serial.println("Wrote block 4 (first 16 bytes of NDEF TLV+record).");

    // Authenticate and write block 5
    if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLen, testBlock + 1, 0 /*Key A*/, defaultKeyA)) {
      Serial.println("Auth failed for block 5. Aborting.");
      delay(1000);
      return;
    }
    if (!nfc.mifareclassic_WriteDataBlock(testBlock + 1, block5)) {
      Serial.println("Write failed for block 5.");
      delay(1000);
      return;
    }
    Serial.println("Wrote block 5 (remainder + terminator).");
    Serial.println("Done. Test with an Android device that supports MIFARE Classic (iPhone won't read Classic).");
  } else {
    // NTAG / Type 2 write path: write 4-byte pages starting at page 4
    // Build full payload bytes: ndef_full (18 bytes) + 0xFE (terminator) => 19 bytes
    uint8_t payload[19];
    memcpy(payload, ndef_full, sizeof(ndef_full)); // 18 bytes
    payload[18] = 0xFE;

    // split into 4-byte pages starting at page 4
    // page 4 -> bytes[0..3], page5 -> bytes[4..7], ...
    uint8_t pageBuf[4];
    const uint8_t startPage = 4;
    const uint8_t totalBytes = sizeof(payload);
    uint8_t pageCount = (totalBytes + 3) / 4; // ceil

    Serial.print("Writing NTAG pages starting at page ");
    Serial.print(startPage);
    Serial.print(" for ");
    Serial.print(pageCount);
    Serial.println(" pages.");

    for (uint8_t p = 0; p < pageCount; ++p) {
      // copy 4 bytes (or pad with 0x00)
      for (int i = 0; i < 4; ++i) {
        uint16_t idx = p * 4 + i;
        pageBuf[i] = (idx < totalBytes) ? payload[idx] : 0x00;
      }
      uint8_t pageNumber = startPage + p;
      if (!nfc.ntag2xx_WritePage(pageNumber, pageBuf)) {
        Serial.print("NTAG write failed on page ");
        Serial.println(pageNumber);
        delay(1000);
        return;
      }
      Serial.print("Wrote NTAG page ");
      Serial.println(pageNumber);
      delay(50); // small pause between page writes
    }
    Serial.println("NTAG write complete — should be readable by Android & iPhone.");
  }

  // prevent rapid repeated writes
  Serial.println("Waiting for tag to be removed...");
  while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 50)) {
    delay(50);
  }
  Serial.println("Tag removed. Ready for next tag.");
  delay(200);
}
