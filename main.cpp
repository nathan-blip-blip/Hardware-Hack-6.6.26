// ============================================================
//  Hand Motion + Distance Detector
//  NUCLEO-G474RE + MPU-6050 + HC-SR04 + 2 LEDs
//  Framework : Mbed OS 6 (bare-metal)
//
//  MPU-6050 (GY-521):
//    VCC  → breadboard + rail (5V from E5V)
//    GND  → breadboard − rail
//    SCL  → D15 (PB_8)
//    SDA  → D14 (PB_9)
//
//  HC-SR04:
//    VCC  → breadboard + rail (5V)
//    GND  → breadboard − rail
//    TRIG → D12 (PA_6)
//    ECHO → D11 (PA_7)
//
//  LEDs (active HIGH, 220Ω to GND rail):
//    BLUE   → D3  (PB_3)   breadboard row 3  anode
//    ORANGE → D6  (PB_10)  breadboard row 12 anode
//
//  Behaviour:
//    STILL + close (<20cm)  → BLUE on
//    STILL + far  (≥20cm)   → ORANGE on
//    Tilt LEFT  or UP       → BLUE on
//    Tilt RIGHT or DOWN     → ORANGE on
//    Fast shake (|a|>1.8g)  → both LEDs flash alternately
//
//  Serial debug: 9600-8-N-1 on COM3
// ============================================================

#include "mbed.h"
#include "mpu6050.h"

// ── Sensors & actuators ─────────────────────────────────────
MPU6050    imu(D14, D15);           // SDA=D14, SCL=D15

DigitalOut trig(D12);               // HC-SR04 trigger
DigitalIn  echo(D11);               // HC-SR04 echo

DigitalOut led_blue  (D3);          // PB_3  — motion LEFT/UP or close
DigitalOut led_orange(D6);          // PB_10 — motion RIGHT/DOWN or far

// ── Serial debug ────────────────────────────────────────────
static BufferedSerial pc(USBTX, USBRX, 9600);

// ── Thresholds ──────────────────────────────────────────────
static const float TILT_THRESHOLD  = 0.30f;   // g
static const float SHAKE_THRESHOLD = 1.80f;   // g
static const float STILL_THRESHOLD = 0.15f;   // g
static const float CLOSE_CM        = 20.0f;   // cm — near/far boundary
static const int   LOOP_MS         = 50;       // 20 Hz

// ── HC-SR04 helper ──────────────────────────────────────────
// Returns distance in cm, or -1.0f on timeout
static float measure_distance_cm() {
    // Send 10 µs trigger pulse
    trig = 0;
    wait_us(2);
    trig = 1;
    wait_us(10);
    trig = 0;

    // Wait for echo to go HIGH (timeout ~30 ms)
    Timer t;
    t.start();
    while (!echo) {
        if (t.elapsed_time() > 30ms) return -1.0f;
    }

    // Measure echo pulse width
    t.reset();
    while (echo) {
        if (t.elapsed_time() > 30ms) return -1.0f;
    }
    auto pulse = t.elapsed_time();
    t.stop();

    // Speed of sound: 343 m/s → 0.0343 cm/µs, round trip ÷2
    float us = (float)duration_cast<std::chrono::microseconds>(pulse).count();
    return us * 0.0343f / 2.0f;
}

// ── LED helpers ─────────────────────────────────────────────
static void both_off() {
    led_blue   = 0;
    led_orange = 0;
}

static void flash_shake(int times = 4) {
    for (int i = 0; i < times; i++) {
        led_blue   = 1; led_orange = 0;
        ThisThread::sleep_for(80ms);
        led_blue   = 0; led_orange = 1;
        ThisThread::sleep_for(80ms);
    }
    both_off();
}

// ── Main ────────────────────────────────────────────────────
int main() {
    printf("\r\n=== Hand Motion + Distance Detector ===\r\n");
    printf("Blue=D3  Orange=D6  TRIG=D12  ECHO=D11\r\n\r\n");

    // Init MPU-6050
    if (!imu.init()) {
        printf("ERROR: MPU-6050 not found!\r\n");
        printf("Check: VCC->+rail, GND->-rail, SCL->D15, SDA->D14\r\n");
        while (true) {
            led_blue = 1; led_orange = 0;
            ThisThread::sleep_for(150ms);
            led_blue = 0; led_orange = 1;
            ThisThread::sleep_for(150ms);
        }
    }
    printf("MPU-6050 OK\r\n");
    ThisThread::sleep_for(150ms);

    while (true) {
        // ── Read MPU-6050 ────────────────────────────────────
        float ax, ay, az;
        imu.read_accel_g(ax, ay, az);
        float mag = sqrtf(ax*ax + ay*ay + az*az);

        // ── Read HC-SR04 ─────────────────────────────────────
        float dist = measure_distance_cm();

        printf("ax=%+.2f ay=%+.2f az=%+.2f |a|=%.2f  dist=%.1fcm\r\n",
               ax, ay, az, mag, dist);

        // ── 1. Shake / impact ────────────────────────────────
        if (mag > SHAKE_THRESHOLD) {
            printf(">> SHAKE — both flash\r\n");
            flash_shake(4);
            ThisThread::sleep_for(300ms);
            continue;
        }

        // ── 2. Tilt detection ────────────────────────────────
        bool tilt_left  = (ax < -TILT_THRESHOLD);
        bool tilt_right = (ax >  TILT_THRESHOLD);
        bool tilt_up    = (ay >  TILT_THRESHOLD);
        bool tilt_down  = (ay < -TILT_THRESHOLD);

        bool is_still = (fabsf(ax) < STILL_THRESHOLD) &&
                        (fabsf(ay) < STILL_THRESHOLD);

        if (!is_still) {
            // Motion overrides distance
            bool blue_on   = tilt_left  || tilt_up;
            bool orange_on = tilt_right || tilt_down;
            led_blue   = blue_on   ? 1 : 0;
            led_orange = orange_on ? 1 : 0;

            if (tilt_left)  printf(">> LEFT  → BLUE\r\n");
            if (tilt_right) printf(">> RIGHT → ORANGE\r\n");
            if (tilt_up)    printf(">> UP    → BLUE\r\n");
            if (tilt_down)  printf(">> DOWN  → ORANGE\r\n");

        } else if (dist > 0.0f) {
            // ── 3. Still — use distance ──────────────────────
            if (dist < CLOSE_CM) {
                led_blue   = 1;
                led_orange = 0;
                printf(">> CLOSE (%.1fcm) → BLUE\r\n", dist);
            } else {
                led_blue   = 0;
                led_orange = 1;
                printf(">> FAR   (%.1fcm) → ORANGE\r\n", dist);
            }
        } else {
            both_off();
            printf(">> STILL, no echo\r\n");
        }

        ThisThread::sleep_for(std::chrono::milliseconds(LOOP_MS));
    }
}
