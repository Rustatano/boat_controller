#include <ESP32Servo.h>
#include <HardwareSerial.h>

#include "../../lib/packet.h"

// pinout
#define TX_GPIO 32
#define RX_GPIO 33
#define MOTOR_BACKWARD_GPIO 25
#define MOTOR_FORWARD_GPIO 26
#define SERVO_GPIO 27

// UART 1, UART 0 is for serial monitor
HardwareSerial Uart1(1);
Servo rudderServo;

// write PWM signal to sevo
void setRudderAngle(uint8_t angle) {
    Serial.print("Angle: ");
    Serial.println(angle);
    rudderServo.write(angle);
}

// write PWM signal and direction to motor
void setThrottle(uint8_t throttle) {
    Serial.println(throttle);
    if (throttle >= 0) {
        analogWrite(MOTOR_FORWARD_GPIO, throttle);
        analogWrite(MOTOR_BACKWARD_GPIO, 0);
    } else {
        analogWrite(MOTOR_FORWARD_GPIO, 0);
        analogWrite(MOTOR_BACKWARD_GPIO, throttle);
    }
}

void setup() {
    Uart1.begin(115200, SERIAL_8N1, RX_GPIO, TX_GPIO);
    Serial.begin(115200);

    pinMode(MOTOR_FORWARD_GPIO, OUTPUT);
    pinMode(MOTOR_BACKWARD_GPIO, OUTPUT);

    // servo setup
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    rudderServo.setPeriodHertz(50);

    rudderServo.attach(SERVO_GPIO, 500, 2400);
    
    // default to middle position, 90 degrees
    rudderServo.write(90);

    // default speed for motor is 0
    analogWrite(MOTOR_FORWARD_GPIO, 0);
    analogWrite(MOTOR_BACKWARD_GPIO, 0);

    Serial.println("E2 started");
}

template <typename T>
void printStructBytesHex(const T& data) {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);
    
    Serial.print("Bytes (HEX): ");
    for (size_t i = 0; i < sizeof(T); i++) {
        if (ptr[i] < 0x10) Serial.print("0"); // Doplnění úvodní nuly pro hodnoty 0-F
        Serial.print(ptr[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void loop() {
    // read data from uart communication
    while (Uart1.available() >= sizeof(ControlPacket)) {
        Serial.println("received something: ");
        // check for correct header
        if ((uint8_t)Uart1.peek() == 0xAA) {
            Serial.println("received correct packet");
            ControlPacket packet;
            Uart1.readBytes((uint8_t*)&packet, sizeof(packet));

            // compare checksums
            uint8_t expectedChecksum = packet.header ^ (uint8_t)packet.throttle ^ packet.rudder_angle;
            if (expectedChecksum == packet.checksum) {
                Serial.println("correct checksum");
                // correct data => able to apply values
                setRudderAngle(packet.rudder_angle);
                setThrottle(packet.throttle);
            }
        } else {
            // incorrect data => try to move by one byte for synchronization
            Uart1.read();
        }
    }
}
