#include <ESP32Servo.h>
#include <HardwareSerial.h>

#include "../../lib/packet.h"

// pinout
#define TX_GPIO 32
#define RX_GPIO 33
#define MOTOR_BACKWARD_GPIO 25
#define MOTOR_FORWARD_GPIO 26
#define SERVO_GPIO 27
// safety stop delay
#define FAILSAFE_TIMEOUT_MS 1000

// UART 1, UART 0 is for serial monitor
HardwareSerial Uart1(1);
Servo rudderServo;

int8_t previous_throttle = 0;

// write PWM signal to servo
void setRudderAngle(uint8_t angle) {
    rudderServo.write(angle);
}

// write PWM signal and direction to motor
void setThrottle(int8_t throttle) {
    // dead zone around 0% throttle, -10 - 10 %
    if (abs(throttle) < 10) {
        analogWrite(MOTOR_FORWARD_GPIO, 0);
        analogWrite(MOTOR_BACKWARD_GPIO, 0);
        previous_throttle = 0;
        return;
    }

    // convert -100 - 100 values to 0 - 255 pwm values
    uint8_t pwm_throttle = map(abs(throttle), 0, 100, 0, 255);

    // kickstarter do make the motor run
    if (previous_throttle == 0) {
        // short 100 % throttles, pwm 255
        if (throttle > 0) {
            analogWrite(MOTOR_FORWARD_GPIO, 255);
            analogWrite(MOTOR_BACKWARD_GPIO, 0);
        } else {
            analogWrite(MOTOR_FORWARD_GPIO, 0);
            analogWrite(MOTOR_BACKWARD_GPIO, 255);
        }

        // kickstart impulse delay
        delay(60);
    }

    if (throttle >= 0) {
        analogWrite(MOTOR_FORWARD_GPIO, pwm_throttle);
        analogWrite(MOTOR_BACKWARD_GPIO, 0);
    } else {
        analogWrite(MOTOR_FORWARD_GPIO, 0);
        analogWrite(MOTOR_BACKWARD_GPIO, pwm_throttle);
    }
}

void setup() {
    Uart1.begin(115200, SERIAL_8N1, RX_GPIO, TX_GPIO);

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
}

void loop() {
    // read data from uart communication
    while (Uart1.available() >= sizeof(ControlPacket)) {
        // check for correct header
        if ((uint8_t)Uart1.peek() == 0xAA) {
            ControlPacket packet;
            Uart1.readBytes((uint8_t*)&packet, sizeof(packet));

            // compare checksums
            uint8_t expectedChecksum = packet.header ^ (uint8_t)packet.throttle ^ packet.rudder_angle;
            if (expectedChecksum == packet.checksum) {
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
