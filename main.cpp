// ============================================================
//  Hand Motion Detector — NUCLEO-G474RE + MPU-6050 + 2 LEDs
//  Framework : Mbed OS 6
//
//  MPU-6050 (GY-521) wiring:
//    VCC  → breadboard + rail (5V from E5V)
//    GND  → breadboard − rail (GND)
//    SCL  → D15 (PB_8)   I2C1 SCL
//    SDA  → D14 (PB_9)   I2C1 SDA
//    AD0  → floating (pulled low on module → addr 0x68)
//
//  LEDs (active HIGH, 220Ω to GND rail):
//    BLUE   → D3  (PB_3)  — lights when hand tilts LEFT or UP
//    ORANGE → D12 (PA_6)  — lights when hand tilts RIGHT or DOWN
//
//  Motion mapping:
//    Tilt LEFT  or UP   → BLUE LED on
//    Tilt RIGHT or DOWN → ORANGE LED on
//    Fast shake (|a|>1.8g) → both LEDs flash alternately
//    Flat / still           → both LEDs off
//
//  Serial debug: 9600-8-N-1 on COM3 (USB Virtual COM)
// ============================================================

#include "mbed.h"
#include "mpu6050.h"

// ── I2C sensor ──────────────────────────────────────────────
MPU6050 imu(D14, D15);          // SDA=D14 (PB_9), SCL=D15 (PB_8)

// ── LEDs (active HIGH) ──────────────────────────────────────
DigitalOut led_blue  (D3);      // PB_3  — tilt LEFT / UP
DigitalOut led_orange(D12);     // PA_6  — tilt RIGHT / DOWN

// ── Serial debug ────────────────────────────────────────────
static BufferedSerial pc(USBTX, USBRX, 9600);

// ── Thresholds ──────────────────────────────────────────────
// Tilt: how many g on X or Y axis counts as a deliberate tilt
static const float TILT_THRESHOLD  = 0.30f;   // g

// Shake: total |acceleration| above this = fast movement/impact
static const float SHAKE_THRESHOLD = 1.80f;   // g

// Dead-band: below this on both axes = hand is flat/still
static const float STILL_THRESHOLD = 0.15f;   // g

// Loop rate
static const int LOOP_MS = 50;                // 20 Hz

// ── Helpers ─────────────────────────────────────────────────
static void both_off() {
    led_blue   = 0;
    led_orange = 0;
}

// Alternate-flash both LEDs for a shake event
static void flash_shake(int times = 4) {
    for (int i = 0; i < times; i++) {
        led_blue   = 1; led_orange = 0;
        ThisThread::sleep_for(80ms);
        led_blue   = 0; led_orange = 1;
        ThisThread::sleep_for(80ms);
    }
    both_off();
}

int main() {
    printf("\r\n=== Hand Motion Detector (2-LED) ===\r\n");
    printf("Blue=D3  Orange=D12\r\n\r\n");

    // ── Init MPU-6050 ────────────────────────────────────────
    if (!imu.init()) {
        printf("ERROR: MPU-6050 not found!\r\n");
        printf("Check: VCC->+rail, GND->-rail, SCL->D15, SDA->D14\r\n");
        // Rapid alternating blink = hardware fault indicator
        while (true) {
            led_blue = 1; led_orange = 0;
            ThisThread::sleep_for(150ms);
            led_blue = 0; led_orange = 1;
            ThisThread::sleep_for(150ms);
        }
    }
    printf("MPU-6050 OK — starting motion detection\r\n\r\n");

    // Short settling time after wake-up
    ThisThread::sleep_for(150ms);

    while (true) {
        float ax, ay, az;
        imu.read_accel_g(ax, ay, az);

        // Total acceleration magnitude
        float mag = sqrtf(ax*ax + ay*ay + az*az);

        printf("ax=%+.2f  ay=%+.2f  az=%+.2f  |a|=%.2f\r\n",
               ax, ay, az, mag);

        // ── 1. Shake / impact detection ──────────────────────
        // |a| spikes well above 1g when the hand moves fast
        if (mag > SHAKE_THRESHOLD) {
            printf(">> SHAKE/IMPACT — both LEDs flash\r\n");
            flash_shake(4);
            ThisThread::sleep_for(300ms);   // debounce
            continue;
        }

        // ── 2. Tilt detection ────────────────────────────────
        // ax > 0  : hand rolls RIGHT  (thumb side up)
        // ax < 0  : hand rolls LEFT   (pinky side up)
        // ay > 0  : fingers point UP  (wrist down)
        // ay < 0  : fingers point DOWN (wrist up)
        //
        // BLUE   lights for LEFT or UP  (ax<0 OR ay>0)
        // ORANGE lights for RIGHT or DOWN (ax>0 OR ay<0)
        // Both off when flat/still

        bool tilt_left  = (ax < -TILT_THRESHOLD);
        bool tilt_right = (ax >  TILT_THRESHOLD);
        bool tilt_up    = (ay >  TILT_THRESHOLD);
        bool tilt_down  = (ay < -TILT_THRESHOLD);

        bool blue_on   = tilt_left  || tilt_up;
        bool orange_on = tilt_right || tilt_down;

        // If both axes are within dead-band → hand is flat → LEDs off
        bool is_still = (fabsf(ax) < STILL_THRESHOLD) &&
                        (fabsf(ay) < STILL_THRESHOLD);

        if (is_still) {
            both_off();
            printf(">> STILL\r\n");
        } else {
            led_blue   = blue_on   ? 1 : 0;
            led_orange = orange_on ? 1 : 0;

            if (tilt_left)  printf(">> LEFT  → BLUE on\r\n");
            if (tilt_right) printf(">> RIGHT → ORANGE on\r\n");
            if (tilt_up)    printf(">> UP    → BLUE on\r\n");
            if (tilt_down)  printf(">> DOWN  → ORANGE on\r\n");
        }

        ThisThread::sleep_for(std::chrono::milliseconds(LOOP_MS));
    }
}
