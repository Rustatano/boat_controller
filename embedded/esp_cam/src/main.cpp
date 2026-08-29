#include <HardwareSerial.h>
#include <WiFi.h>
#include <ArduinoJson.h>

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
httpd_handle_t server_handle = NULL;
int last_client_fd = -1;

// UART 1, UART 0 is for serial monitor
HardwareSerial Uart1(1);

// send data over uart communication
void sendControlPacket(int8_t throttle, uint8_t rudder_angle) {
    ControlPacket packet;
    packet.throttle = throttle;
    packet.rudder_angle = rudder_angle;
    packet.checksum = packet.header ^ (uint8_t)packet.throttle ^ packet.rudder_angle;

    Uart1.write((uint8_t*)&packet, sizeof(packet));
    Serial.println("sent something");
}

// send data over Wi-Fi to controller
void sendTelemetryData(int8_t speed, int8_t temperature, uint8_t water_leak) {
    if (server_handle == NULL || last_client_fd < 0) return;

    // json preparaion
    JsonDocument doc;
    doc["speed"] = speed;
    doc["temp"] = temperature;
    doc["leak"] = water_leak;

    // serialize json
    String jsonString;
    serializeJson(doc, jsonString);

    // ws frame preparation
    httpd_ws_frame_t ws_packet;
    memset(&ws_packet, 0, sizeof(httpd_ws_frame_t));
    ws_packet.payload = (uint8_t*)jsonString.c_str();
    ws_packet.len = jsonString.length();
    ws_packet.type = HTTPD_WS_TYPE_TEXT;

    // async send to controller
    httpd_ws_send_frame_async(server_handle, last_client_fd, &ws_packet);
}

// websocket handler
esp_err_t ws_handler(httpd_req_t *req) {
    // handshake
    Serial.println("handshake");
    if (req->method == HTTP_GET) {
        last_client_fd = httpd_req_to_sockfd(req);
        return ESP_OK;
    }
    
    httpd_ws_frame_t ws_packet;
    memset(&ws_packet, 0, sizeof(httpd_ws_frame_t));
    
    Serial.println("len data");
    // get length of incoming frame
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_packet, 0);
    if (ret != ESP_OK || ws_packet.len == 0) {
        return ret;
    }

    Serial.println("alloc data");
    // allocate space for text data + '\n'
    char *buf = (char*) malloc(ws_packet.len + 1);
    if (buf == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ws_packet.payload = (uint8_t*) buf;
    
    // load data from frame
    ret = httpd_ws_recv_frame(req, &ws_packet, ws_packet.len);
    Serial.println("load data");
    if (ret == ESP_OK) {
        // string ending
        buf[ws_packet.len] = '\0';

        // deserialize into json
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, buf);

        if (!error) {
            int8_t throttle = doc["throttle"];
            uint8_t rudder_angle = doc["rudder_angle"];

            // send control data over uart
            sendControlPacket(throttle, rudder_angle);
        }
    }

    free(buf);

    return ret;
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

// http server init
void startCameraServer() {
    // http server config and port
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    // page path handlers
    httpd_uri_t stream_uri = {.uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL};
    httpd_uri_t ws_uri = {.uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .user_ctx = NULL, .is_websocket = true};

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        httpd_register_uri_handler(stream_httpd, &ws_uri);
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

    // 10 MHz for stability
    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound()) {
        // 640x480
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 12;
        config.fb_count = 2;
    } else {
        // 320x240
        config.frame_size = FRAMESIZE_QVGA;
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

void loop() {
    // read data from uart communication
    while (Uart1.available() >= sizeof(TelemetryPacket)) {
        // check for correct header
        if (Uart1.peek() == 0xBB) {
            TelemetryPacket packet;
            Uart1.readBytes((uint8_t*)&packet, sizeof(packet));

            // compare checksums
            if ((packet.header ^ packet.speed ^ packet.temperature ^ packet.water_leak) == packet.checksum) {
                // correct data => able to send it to controller
                sendTelemetryData(packet.speed, packet.temperature, packet.water_leak);
            }
        } else {
            // incorrect data => try to move by one byte for synchronization
            Uart1.read();
        }
    }
}