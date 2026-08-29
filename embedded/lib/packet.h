#include <cstdint>

struct __attribute__((__packed__)) ControlPacket {
    uint8_t header = 0xAA;
    int8_t throttle; // 0 - 100 percent
    uint8_t rudder_angle; // 0 - 180 degrees
    uint8_t checksum;
};

struct __attribute__((__packed__)) TelemetryPacket {
    uint8_t header = 0xBB;
    int8_t speed; // -128 - 127 cm/s
    int8_t temperature; // -40 - 125 degrees Celsius
    uint8_t water_leak; // 0 = no leak, 1 = leak (danger)
    uint8_t checksum;
};

