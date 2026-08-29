#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <HardwareSerial.h>

#include "../../lib/packet.h"
#include "esp_camera.h"

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
const char* ssid = "BoatController";
const char* password = "NoExplosionPlease";

// create server and websocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

HardwareSerial Uart1(1);

// send control data over uart to control esp
void sendControlPacket(int8_t throttle, uint8_t rudder_angle) {
    ControlPacket packet;
    packet.throttle = throttle;
    packet.rudder_angle = rudder_angle;
    packet.checksum = packet.header ^ (uint8_t)packet.throttle ^ packet.rudder_angle;

    Uart1.write((uint8_t*)&packet, sizeof(packet));
    Serial.println("sent something over UART");
}

// send telemetry data over Wi-Fi to client app
void sendTelemetryData(int8_t speed, int8_t temperature, uint8_t water_leak) {
    if (ws.count() == 0) {
        // no clients connected
        return;
    }

    JsonDocument doc;
    doc["speed"] = speed;
    doc["temp"] = temperature;
    doc["leak"] = water_leak;

    String jsonString;
    serializeJson(doc, jsonString);

    // send data through websocket
    ws.textAll(jsonString);
}

// incoming websocket messages handler
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("websocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("websocket client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            // string ending
            data[len] = '\0';

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (char*)data);

            if (!error) {
                int8_t throttle = doc["throttle"];
                uint8_t rudder_angle = doc["rudder_angle"];

                sendControlPacket(throttle, rudder_angle);
            }
        }
    }
}

// video stream handler
void handleMjpegStream(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse("multipart/x-mixed-replace; boundary=frame", 0, 
        [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            static camera_fb_t * fb = NULL;
            static size_t frameIndex = 0;

            if (!fb) {
                fb = esp_camera_fb_get();
                if (!fb) {
                    return 0;
                }
                frameIndex = 0;
            }

            char header[128];
            int hlen = snprintf(header, sizeof(header), "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
            size_t totalFrameLen = hlen + fb->len + 2;

            size_t bytesWritten = 0;

            while (bytesWritten < maxLen && frameIndex < totalFrameLen) {
                if (frameIndex < (size_t)hlen) {
                    // send HTTP header
                    buffer[bytesWritten++] = header[frameIndex++];
                } 
                else if (frameIndex < hlen + fb->len) {
                    // send jpeg image data
                    buffer[bytesWritten++] = fb->buf[frameIndex - hlen];
                    frameIndex++;
                } 
                else {
                    // send ending \r\n
                    buffer[bytesWritten++] = (frameIndex == hlen + fb->len) ? '\r' : '\n';
                    frameIndex++;
                }
            }

            // Pokud jsme poslali celý snímek, uvolníme ho a připravíme se na další
            if (frameIndex >= totalFrameLen) {
                esp_camera_fb_return(fb);
                fb = NULL;
                frameIndex = 0;
            }

            return bytesWritten;
        }
    );

    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Cache-Control", "no-cache, private");
    response->addHeader("Pragma", "no-cache");
    request->send(response);
}

void setup() {
    Serial.begin(115200);
    Uart1.begin(115200, SERIAL_8N1, RX_GPIO, TX_GPIO);

    // camera config
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
    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound()) {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 12;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("ERR: camera initialization failed");
        return;
    }

    WiFi.softAP(ssid, password);

    // registration of websocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // registration of HTTP stream
    server.on("/stream", HTTP_GET, handleMjpegStream);

    server.begin();
    Serial.println("LOG: async server and websocket running");
}

void loop() {
    // resotring websocket state
    ws.cleanupClients();

    // read telemetry data from uart communication from control esp
    while (Uart1.available() >= sizeof(TelemetryPacket)) {
        if (Uart1.peek() == 0xBB) {
            TelemetryPacket packet;
            Uart1.readBytes((uint8_t*)&packet, sizeof(packet));

            if ((packet.header ^ packet.speed ^ packet.temperature ^ packet.water_leak) == packet.checksum) {
                sendTelemetryData(packet.speed, packet.temperature, packet.water_leak);
            }
        } else {
            Uart1.read();
        }
    }
}