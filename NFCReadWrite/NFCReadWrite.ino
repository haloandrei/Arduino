#include <SPI.h>
#include <Adafruit_PN532.h>

// ===== ESP32 SPI pins (VSPI) =====
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23
#define PIN_SPI_SS     5

// Use hardware SPI; the Adafruit PN532 driver only needs SS here
Adafruit_PN532 nfc(PIN_SPI_SS);

// ---- Change this to your URL (keep it short; NTAG213 ~144B total user memory) ----
// We use NDEF URI prefix 0x03 = "https://"
const char* URL_NO_PREFIX = "www.deblocat.ro";

// Start page for user data on NTAG213/Ultralight
const uint8_t FIRST_DATA_PAGE = 4;

// Max user bytes on NTAG213 = 144 (pages 4..39 inclusive)
const size_t NTAG213_USER_BYTES = 144;

// Build a simple NDEF URI record with TLV wrapper:
// TLV: [0x03][len][NDEF bytes][0xFE]
// NDEF (short record): [MB/ME/SR TNF=0x01][type_len=1]["U"][payload_len][uri_prefix][url...]
//
// uri_prefix code 0x03 = "https://"
// (Other codes exist if you want additional compression)
void buildNdefUriTlv(const char* urlNoPrefix, uint8_t* outBuf, size_t& outLen) {
  // Compose NDEF record
  const uint8_t NDEF_HEADER = 0b11010001; // MB=1, ME=1, CF=0, SR=1, IL=0, TNF=0x01 (Well-known)
  const uint8_t TYPE_LEN = 1;             // 'U'
  const uint8_t TYPE_FIELD = 'U';
  const uint8_t URI_PREFIX = 0x03;        // "https://"

  const size_t urlLen = strlen(urlNoPrefix);
  const uint8_t PAYLOAD_LEN = (uint8_t)(1 + urlLen); // 1 byte prefix + URL

  // NDEF bytes length (header + type_len + payload_len + type + payload)
  const size_t NDEF_LEN = 1 + 1 + 1 + 1 + PAYLOAD_LEN;

  // TLV: 0x03, length, NDEF..., 0xFE
  size_t i = 0;
  outBuf[i++] = 0x03;               // NDEF TLV
  outBuf[i++] = (uint8_t)NDEF_LEN;  // short length (fits in 1 byte for short messages)

  // NDEF record
  outBuf[i++] = NDEF_HEADER;
  outBuf[i++] = TYPE_LEN;
  outBuf[i++] = PAYLOAD_LEN;
  outBuf[i++] = TYPE_FIELD;
  outBuf[i++] = URI_PREFIX;

  // URL (no prefix)
  for (size_t k = 0; k < urlLen; k++) {
    outBuf[i++] = (uint8_t)urlNoPrefix[k];
  }

  // Terminator TLV
  outBuf[i++] = 0xFE;

  outLen = i;
}

// Write the given buffer into NTAG213 user pages starting at page 4.
// Pads remaining bytes with 0x00.
bool writeUltralightPages(const uint8_t* data, size_t len) {
  // NTAG213 user memory: 144 bytes across pages 4..39 (36 pages * 4 bytes)
  // We’ll fill as many pages as needed, then 0x00 pad the rest of the last page.
  uint8_t page = FIRST_DATA_PAGE;
  size_t pos = 0;

  while (pos < len) {
    uint8_t block[4] = {0x00, 0x00, 0x00, 0x00};
    for (int b = 0; b < 4 && pos < len; b++, pos++) {
      block[b] = data[pos];
    }

    if (!nfc.ntag2xx_WritePage(page, block)) {
      Serial.printf("Write failed at page %u\n", page);
      return false;
    }
    page++;

    // Avoid overrunning user memory on NTAG213
    if ((page - FIRST_DATA_PAGE) * 4 > NTAG213_USER_BYTES) {
      Serial.println("Data too large for NTAG213 user memory.");
      return false;
    }
  }

  // (Optional) If you want to blank the rest of the current page block, we already pad zeros.
  // If you want to blank *all remaining pages*, add a loop here (not necessary for NDEF).

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(10000);
  Serial.println("App started");
  pinMode(2,OUTPUT);
  digitalWrite(2,0);
  // Explicitly init SPI pins on ESP32
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_SS);
  delay(1000);
  pinMode(2,OUTPUT);
  digitalWrite(2,1);
  nfc.begin();
  // Recommended setup for PN532
  nfc.SAMConfig(); // configure board to read tags

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN532. Check wiring and SPI mode.");
    while (1) { delay(1000); }
  }
  Serial.printf("PN532 detected. IC: 0x%02X  Ver: %u.%u  Support: 0x%02X\n",
                (uint8_t)(versiondata >> 24),
                (uint8_t)(versiondata >> 16) & 0xFF,
                (uint8_t)(versiondata >> 8) & 0xFF,
                (uint8_t)versiondata & 0xFF);

  Serial.println("Bring an NTAG213 close to the antenna to WRITE the URL...");
}

void loop() {
  // Look for an ISO14443A tag (NTAG/Mifare Ultralight/Classic) for up to 1000 ms
  uint8_t uid[7];
  uint8_t uidLength = 0;
  bool found = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000);

  if (!found) {
    // No tag yet; keep polling
    return;
  }

  Serial.print("Tag detected. UID: ");
  for (uint8_t i = 0; i < uidLength; i++) {
    Serial.printf("%02X", uid[i]);
  }
  Serial.println();

  // Check tag type supports NTAG/Ultralight commands
  // (Ultralight/NTAG don’t need auth for writing user pages)
  // Optional: You could call nfc.ntag2xx_ReadPage(4, buf) to sanity check.

  // Build TLV for our URL
  uint8_t tlv[NTAG213_USER_BYTES]; // enough buffer
  size_t tlvLen = 0;
  buildNdefUriTlv(URL_NO_PREFIX, tlv, tlvLen);

  // Safety: ensure it fits
  if (tlvLen > NTAG213_USER_BYTES) {
    Serial.println("NDEF message too large for NTAG213.");
    delay(1500);
    return;
  }

  Serial.printf("Writing %u bytes of TLV/NDEF starting at page %u...\n", (unsigned)tlvLen, FIRST_DATA_PAGE);

  // Program the tag
  if (!writeUltralightPages(tlv, tlvLen)) {
    Serial.println("❌ Write failed. Hold tag steady and try again.");
    delay(1500);
    return;
  }

  Serial.println("✅ NDEF URL written successfully!");
  Serial.println("Try reading it with your phone (NFC on).");

  // Debounce so you don’t immediately rewrite if tag stays in field
  delay(2000);
}
