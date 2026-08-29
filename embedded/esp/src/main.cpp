#include <ESP32Servo.h>
#include <HardwareSerial.h>

#include "../../lib/packet.h"

// pinout
#define TX_GPIO 12
#define RX_GPIO 13
#define MOTOR_BACKWARD_GPIO 25
#define MOTOR_FORWARD_GPIO 26
#define SERVO_GPIO 27

// constants
// 1 kHz
#define PWM_fREQUENCY 1000
// 8 bits => 0 - 255 possible values
#define PWM_RESOLUTION 8

// UART 1, UART 0 is for serial monitor
HardwareSerial Uart1(1);
Servo rudderServo;

// write PWM signal to sevo
void setRudderAngle(uint8_t angle) {
    rudderServo.write(angle);
}

// write PWM signal and direction to motor
void setThrottle(uint8_t throttle) {
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

    rudderServo.attach(SERVO_GPIO);
    
    // default to middle position, 90 degrees
    rudderServo.write(90);

    // default speed for motor is 0
    analogWrite(MOTOR_FORWARD_GPIO, 0);
    analogWrite(MOTOR_BACKWARD_GPIO, 0);

    Serial.println("E2 started");
}

void loop() {
    // read data from uart communication
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
