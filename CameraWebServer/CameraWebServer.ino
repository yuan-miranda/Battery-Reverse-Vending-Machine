#include "esp_camera.h"
#include <WiFi.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include <HTTPClient.h>

// ===========================
// Select camera model in board_config.h
// ===========================
#include "board_config.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "ESP32_S3_CAM";
const char *password = "12345678";
const char *laptop_ip = "192.168.4.2";

Preferences prefs;

int coin1Left = 0;
int coin5Left = 0;

Servo coin1Servo;
Servo coin5Servo;
Servo acceptRejectServo;
#define COIN1_SERVO_PIN 21
#define COIN5_SERVO_PIN 47
#define ACCEPT_REJECT_SERVO_PIN 48
#define SCAN_PIN 1

void startCameraServer();
void setupLedFlash();

void triggerScan()
{
    if (WiFi.softAPgetStationNum() > 0)
    {
        HTTPClient http;
        String url = "http://" + String(laptop_ip) + ":5000/scan";

        http.begin(url);
        int httpResponseCode = http.GET();

        if (httpResponseCode <= 0)
        {
            Serial.printf("Error on HTTP request: %s", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    pinMode(SCAN_PIN, INPUT_PULLUP);
    coin1Servo.attach(COIN1_SERVO_PIN);
    coin5Servo.attach(COIN5_SERVO_PIN);
    acceptRejectServo.attach(ACCEPT_REJECT_SERVO_PIN);

    coin1Servo.write(0);
    coin5Servo.write(0);
    acceptRejectServo.write(180);

    prefs.begin("data", false);
    coin1Left = prefs.getInt("coin1Left", 0);
    coin5Left = prefs.getInt("coin5Left", 0);

    camera_config_t config;
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
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
    //                      for larger pre-allocated frame buffer.
    if (config.pixel_format == PIXFORMAT_JPEG)
    {
        if (psramFound())
        {
            config.frame_size = FRAMESIZE_QVGA;
            config.jpeg_quality = 20;
            config.fb_count = 1;
            config.grab_mode = CAMERA_GRAB_LATEST;
        }
        else
        {
            // Limit the frame size when PSRAM is not available
            config.frame_size = FRAMESIZE_SVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    }
    else
    {
        // Best option for face detection/recognition
        config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
        config.fb_count = 1;
#endif
    }

#if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
#endif

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID)
    {
        s->set_vflip(s, 1);       // flip it back
        s->set_brightness(s, 1);  // up the brightness just a bit
        s->set_saturation(s, -2); // lower the saturation
    }
    // drop down frame size for higher initial frame rate
    if (config.pixel_format == PIXFORMAT_JPEG)
    {
        s->set_framesize(s, FRAMESIZE_QVGA);
    }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
    s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
    setupLedFlash();
#endif

    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();

    startCameraServer();
    Serial.print("http://");
    Serial.println(IP);
}

void loop()
{
    if (digitalRead(SCAN_PIN) == LOW)
    {
        triggerScan();
        delay(1000);

        while (digitalRead(SCAN_PIN) == LOW)
        {
            delay(100);
        }
    }

    delay(10);
}

void coinPressed(int value)
{
    if (value == 1)
    {
        coin1Servo.write(180);
        delay(1000);
        coin1Servo.write(0);
    }
    else if (value == 5)
    {
        coin5Servo.write(180);
        delay(1000);
        coin5Servo.write(0);
    }
}

void acceptPressed(int value)
{
    acceptRejectServo.write(135);
    // enough time to slide
    delay(1000);
    acceptRejectServo.write(180);

    if (value == 1)
    {
        coinPressed(1);
    }
    else if (value == 5)
    {
        coinPressed(5);
    }
}

void rejectPressed()
{
    acceptRejectServo.write(90);
    // full drop
    delay(500);
    acceptRejectServo.write(180);
}