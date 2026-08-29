// made by Gemini Flash, edited

#include <HardwareSerial.h>
#include <WiFi.h>

#include "../../lib/packet.h"
#include "esp_camera.h"
#include "esp_http_server.h"

// pinout
#define TX_GPIO 12
#define RX_GPIO 13

#define PWDN_GPIO 32
#define RESET_GPIO -1
#define XCLK_GPIO 0
#define SIOD_GPIO 26
#define SIOC_GPIO 27
#define Y9_GPIO 35
#define Y8_GPIO 34
#define Y7_GPIO 39
#define Y6_GPIO 36
#define Y5_GPIO 21
#define Y4_GPIO 19
#define Y3_GPIO 18
#define Y2_GPIO 5
#define VSYNC_GPIO 25
#define HREF_GPIO 23
#define PCLK_GPIO 22

// Wi-Fi credentials
const char* ssid = "BoatControlller";
const char* password = "NoExplosionPlease";

httpd_handle_t stream_httpd = NULL;

// UART 1, UART 0 is for serial monitor
HardwareSerial Uart1(1);

void sendControlPacket(int8_t throttle, uint8_t rudder_angle) {
    ControlPacket packet;
    packet.throttle = throttle;
    packet.rudder_angle = rudder_angle;
    packet.checksum = packet.header ^ packet.throttle ^ packet.rudder_angle;

    Uart1.write((uint8_t*)&packet, sizeof(packet));
}

// mjpeg stream handler
esp_err_t stream_handler(httpd_req_t* req) {
    camera_fb_t* frame_buf = NULL;
    char part_buf[128];
    // image boundary separator
    esp_err_t res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=123456789000000000000987654321");

    if (res != ESP_OK) {
        return res;
    }

    // http header config
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, private, max-age=0, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");

    while (true) {
        // get frame buffer
        frame_buf = esp_camera_fb_get();
        if (!frame_buf) {
            Serial.println("ERR: Failed to capture a frame");
            res = ESP_FAIL;
            break;
        }

        // text image-descriptor preparation
        size_t hlen = snprintf(part_buf, 128,
                               "\r\n--123456789000000000000987654321\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                               frame_buf->len);

        // send image text-description
        res = httpd_resp_send_chunk(req, part_buf, hlen);

        // send image data
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char*)frame_buf->buf, frame_buf->len);
        }

        // free frame buffer for next frame
        esp_camera_fb_return(frame_buf);
        frame_buf = NULL;

        if (res != ESP_OK) {
            break;
        }

        // safety delay
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    return res;
}

// clean raw html stream-only page
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Kamerový Stream</title>
  <style>
    body { 
      margin: 0; 
      padding: 0; 
      background-color: #000; 
      display: flex; 
      justify-content: center; 
      align-items: center; 
      height: 100vh; 
      overflow: hidden;
    }
    img { 
      max-width: 100%; 
      max-height: 100%; 
      object-fit: contain; 
    }
  </style>
</head>
<body>
  <img src="/stream" id="stream">
</body>
</html>
)rawliteral";

// http page index handler
esp_err_t index_handler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
}

// http server init
void startCameraServer() {
    // http server config and port
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    // page path handlers
    httpd_uri_t index_uri = {.uri = "/", .method = HTTP_GET, .handler = index_handler, .user_ctx = NULL};
    httpd_uri_t stream_uri = {.uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL};

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("LOG: HTTP server started");
    }
}

void setup() {
    Serial.begin(115200);
    Uart1.begin(115200, SERIAL_8N1, RX_GPIO, TX_GPIO);

    // pin assignment
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO;
    config.pin_d1 = Y3_GPIO;
    config.pin_d2 = Y4_GPIO;
    config.pin_d3 = Y5_GPIO;
    config.pin_d4 = Y6_GPIO;
    config.pin_d5 = Y7_GPIO;
    config.pin_d6 = Y8_GPIO;
    config.pin_d7 = Y9_GPIO;
    config.pin_xclk = XCLK_GPIO;
    config.pin_pclk = PCLK_GPIO;
    config.pin_vsync = VSYNC_GPIO;
    config.pin_href = HREF_GPIO;
    config.pin_sccb_sda = SIOD_GPIO;
    config.pin_sccb_scl = SIOC_GPIO;
    config.pin_pwdn = PWDN_GPIO;
    config.pin_reset = RESET_GPIO;

    config.xclk_freq_hz = 10000000;  // 10 MHz for stability
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound()) {
        config.frame_size = FRAMESIZE_VGA;  // 640x480
        config.jpeg_quality = 12;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_QVGA;  // 320x240
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    // initializing camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("ERR: camera error: 0x%x\n", err);
        return;
    }

    // first frame cleaning
    camera_fb_t* frame_buf = esp_camera_fb_get();
    if (frame_buf) {
        esp_camera_fb_return(frame_buf);
    }

    // initializing Wi-Fi
    WiFi.softAP(ssid, password);

    // initializing http server
    startCameraServer();

    Serial.print("Stream accesible at Wi-Fi 'BoatController': http://");
    Serial.println(WiFi.softAPIP());
}

void sendTelemetryData(TelemetryPacket* packet) {

}

void readSerialData() {
  while (Serial.available() >= sizeof(TelemetryPacket)) {
    // check for correct header
    if (Serial.peek() == 0xBB) {
      TelemetryPacket packet;
      Serial.readBytes((uint8_t*)&packet, sizeof(packet));

      // compare checksums
      if (packet.header ^ packet.speed ^ packet.temperature ^ packet.water_leak == packet.checksum) {
        // correct data => able to send it to controller
        sendTelemetryData(&packet);
      }
    } else {
      // incorrect data => try to move by one byte for synchronization
      Serial.read();
    }
  }
}

void loop() {
}