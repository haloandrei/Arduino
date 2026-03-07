#include "esp_camera.h"
#include <WiFi.h>
#include "board_config.h"

// --- ADDED: Display Libraries ---
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 
// Define our safe I2C pins
#define I2C_SDA 33
#define I2C_SCL 32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// --------------------------------

#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
#include "camera_pins.h"

camera_config_t config;
void camera_init();

void setup() {
  Serial.begin(115200);
  
  // --- ADDED: Initialize I2C and Display ---
  Wire.begin(I2C_SDA, I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is standard I2C address
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Camera Init...");
  display.display();
  // -----------------------------------------

  camera_init();

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 0);        
  s->set_hmirror(s, 0);      
  s->set_brightness(s, 1);   
  s->set_saturation(s, -1);  

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Camera Ready!");
  display.display();
  delay(1000); // Pause to read the message
}

void loop() {
  // --- ADDED: Capture frame and draw to OLED ---
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  display.clearDisplay();
  
  // The camera is 160x120 (QQVGA), the screen is 128x64. 
  // We do a simple downscale and threshold mapping.
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      
      // Map screen coordinates to camera coordinates
      int cam_x = (x * 160) / SCREEN_WIDTH;
      int cam_y = (y * 120) / SCREEN_HEIGHT;
      
      // Grab the grayscale pixel value (0-255)
      uint8_t pixel_brightness = fb->buf[(cam_y * 160) + cam_x];
      
      // Threshold: If it's bright enough, draw a white pixel
      if (pixel_brightness > 120) { // Adjust this value (0-255) to change contrast
        display.drawPixel(x, y, SSD1306_WHITE);
      }
    }
  }
  
  display.display(); // Push the drawing to the screen
  esp_camera_fb_return(fb); // Return the buffer so the camera can take the next frame
  // ----------------------------------------------
}

void camera_init() {
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  
  // --- CHANGED FOR OLED COMPATIBILITY ---
  config.frame_size = FRAMESIZE_QQVGA;       // 160x120, small enough for the ESP to process quickly
  config.pixel_format = PIXFORMAT_GRAYSCALE; // Much easier to convert to black/white than JPEG
  // --------------------------------------
  
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;
}