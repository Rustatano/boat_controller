#include <cstdint>

struct __attribute__((__packed__)) ControlPacket {
    uint8_t header = 0xAA;
    // -100 - 100 percent
    int8_t throttle;
    // 0 - 180 degrees
    uint8_t rudder_angle;
    uint8_t checksum;
};

struct __attribute__((__packed__)) TelemetryPacket {
    uint8_t header = 0xBB;
    // -128 - 127 cm/s
    int8_t speed;
    // -40 - 125 degrees Celsius
    int8_t temperature;
    // 0 = no leak, 1 = leak (danger)
    uint8_t water_leak;
    uint8_t checksum;
};
