/**
 * receiver_candle_led_robust_pinAPI.ino
 * Uses ledcAttach(pin, freq, bits) + ledcWrite(pin, duty).
 */
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <math.h>                    // sin(), pow()

/* ------------ user tweaks ----------------------------------- */
constexpr uint8_t    WIFI_CHANNEL   = 1;
constexpr gpio_num_t LED_PIN        = GPIO_NUM_4;   // any LEDC-capable GPIO

constexpr uint32_t LEDC_FREQ        = 5000;         // Hz
constexpr uint8_t  LEDC_BITS        = 12;           // 0-4095 duty

constexpr uint32_t PERIOD_MS        = 4000;         // full breathe cycle
constexpr float    GAMMA            = 2.2;
constexpr float    BR_MIN           = 0.08;         // min visible brightness
constexpr float    BR_MAX           = 1.00;
/* ------------------------------------------------------------ */

volatile bool candleRunning = false;                // set by ESP-NOW ISR

inline uint16_t gammaCorrect(float lin)             // 0-1 → 0-4095
{
  lin = fminf(fmaxf(lin, 0.f), 1.f);
  return uint16_t(powf(lin, GAMMA) * ((1 << LEDC_BITS) - 1));
}

/* ---------------- ESP-NOW callback -------------------------- */
void IRAM_ATTR onReceive(const esp_now_recv_info_t*,
                         const uint8_t* d, int len)
{
  if (len == 1) {
    candleRunning = d[0];
    if (!candleRunning) ledcWrite(LED_PIN, 0);      // OFF instantly
  }
}

void setup()
{
  Serial.begin(115200);

  /* 1. PWM – simple one-liner */
  ledcAttach(LED_PIN, LEDC_FREQ, LEDC_BITS);
  ledcWrite (LED_PIN, 0);

  /* 2. Wi-Fi + ESP-NOW */
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  ESP_ERROR_CHECK(esp_now_init());
  esp_now_register_recv_cb(onReceive);

  Serial.println("Robust candle (pin API) ready – MAC");
  Serial.println( WiFi.macAddress());
}

void loop()
{
  if (!candleRunning) { delay(10); return; }

  static uint32_t t0 = millis();                   // LFO anchor
  float phase = float((millis() - t0) % PERIOD_MS) / PERIOD_MS; // 0-1

  /* triangle → sine ease */
  float tri  = phase < 0.5f ? phase * 2.f : (1.f - phase) * 2.f;
  float sine = sinf(tri * M_PI_2);

  float lin  = BR_MIN + sine * (BR_MAX - BR_MIN);
  ledcWrite(LED_PIN, gammaCorrect(lin));

  delay(1);                                        // 1 kHz update, non-blocking
}
