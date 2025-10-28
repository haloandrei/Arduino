/**
 * pot_reader_gpio4.ino
 *
 * ▸ Potentiometer ends → 3 V3 and GND
 * ▸ Wiper (centre pin) → GPIO 4   (ADC2_CH0)
 *
 * NOTE  ▸ GPIO 4 sits on ADC-2.  Wi-Fi or Bluetooth must be OFF while
 *         you read it; Wi-Fi is disabled below with WiFi.mode(WIFI_OFF).
 */

#include <Arduino.h>
#include <WiFi.h>          // only for WiFi.mode(); no radio will run

constexpr gpio_num_t POT_PIN     = GPIO_NUM_34;
constexpr uint16_t   READ_PERIOD = 100;      // ms between prints

void setup()
{
  Serial.begin(115200);

  /* Disable Wi-Fi/BLE so ADC-2 is free */
  WiFi.mode(WIFI_OFF);
  btStop();                       // turn off BLE stack as well

  /* ADC set-up */
  analogReadResolution(12);       // 0-4095
  analogSetPinAttenuation(POT_PIN, ADC_11db);   // full 0-3.3 V span

  Serial.println(F("Reading potentiometer on GPIO 4 …"));
  Serial.println(F("RawADC,Angle"));
}

void loop()
{
  static uint32_t prev = 0;
  if (millis() - prev >= READ_PERIOD) {
    prev = millis();

    uint16_t raw   = analogRead(POT_PIN);        // 0-4095
    uint8_t  angle = map(raw, 0, 4095, 0, 180);  // 0-180 °

    /* CSV-style output: easy to paste into a spreadsheet */
    Serial.print(raw);
    Serial.print(',');
    Serial.println(angle);
  }
}
