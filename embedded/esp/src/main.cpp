#include <HardwareSerial.h>

#include "../../lib/packet.h"

#define TX_GPIO 12
#define RX_GPIO 13

// UART 1, UART 0 is for serial monitor
HardwareSerial Uart1(1);

void setRudderAngle(uint8_t angle) {
}

void setThrottle(uint8_t throttle) {
}

void readSerialData() {
    while (Serial.available() >= sizeof(ControlPacket)) {
        // check for correct header
        if (Serial.peek() == 0xBB) {
            ControlPacket packet;
            Serial.readBytes((uint8_t*)&packet, sizeof(packet));

            // compare checksums
            if (packet.header ^ packet.throttle ^ packet.rudder_angle == packet.checksum) {
                // correct data => able to apply values
                setRudderAngle(packet.rudder_angle);
                setThrottle(packet.throttle);
            }
        } else {
            // incorrect data => try to move by one byte for synchronization
            Serial.read();
        }
    }
}

void setup() {
    Uart1.begin(115200, SERIAL_8N1, RX_GPIO, TX_GPIO);
    Serial.begin(115200);
    Serial.println("E2 started");
}

void loop() {
}
