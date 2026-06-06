"""
Nucleo G474RE MicroPython
==========================
Components:
  - MPU-6050      → I2C1: SDA=D14 (PB9), SCL=D15 (PB8)
  - HC-SR04       → TRIG=D8 (PA9), ECHO=D7 (PA8)
    ⚠ ECHO is 5V — use 1kΩ + 2kΩ voltage divider before D7
  - STEMMA Speaker→ D5 (PB4)

Behaviour:
  - HC-SR04 detects object within range  → speaker plays tone
  - MPU-6050 detects motion (board moved) → speaker silenced
  - Both conditions must be true to hear sound:
      object detected AND board is still
"""

import machine
import time
import math
import struct

# ── I2C for MPU-6050 ──────────────────────────────────────────
i2c = machine.I2C(1, scl=machine.Pin('PB8'), sda=machine.Pin('PB9'), freq=400_000)

# ── HC-SR04 ───────────────────────────────────────────────────
TRIG = machine.Pin('PA9', machine.Pin.OUT)
ECHO = machine.Pin('PA8', machine.Pin.IN)

# ── STEMMA Speaker ────────────────────────────────────────────
spk_pin   = machine.Pin('PB4', machine.Pin.OUT)
spk_timer = machine.Timer()

# ── MPU-6050 ──────────────────────────────────────────────────

MPU_ADDR    = 0x68
ACCEL_SCALE = 16384.0

# How much acceleration change counts as "motion" (in g)
MOTION_THRESHOLD = 0.08


def mpu_init():
    i2c.writeto_mem(MPU_ADDR, 0x6B, b'\x00')  # wake up
    time.sleep_ms(100)
    i2c.writeto_mem(MPU_ADDR, 0x1C, b'\x00')  # ±2g range


def mpu_read_accel():
    raw = i2c.readfrom_mem(MPU_ADDR, 0x3B, 6)
    ax, ay, az = struct.unpack('>hhh', raw)
    return ax / ACCEL_SCALE, ay / ACCEL_SCALE, az / ACCEL_SCALE


# ── HC-SR04 ───────────────────────────────────────────────────

DETECT_RANGE_CM = 50.0   # object must be closer than this to trigger sound


def measure_cm():
    """Return distance in cm, or None on timeout."""
    TRIG.value(0)
    time.sleep_us(2)
    TRIG.value(1)
    time.sleep_us(10)
    TRIG.value(0)

    t_out = time.ticks_add(time.ticks_us(), 30_000)
    while ECHO.value() == 0:
        if time.ticks_diff(t_out, time.ticks_us()) <= 0:
            return None

    t_start = time.ticks_us()
    t_out   = time.ticks_add(t_start, 30_000)
    while ECHO.value() == 1:
        if time.ticks_diff(t_out, time.ticks_us()) <= 0:
            return None

    return (time.ticks_diff(time.ticks_us(), t_start) * 0.0343) / 2.0


# ── Speaker ───────────────────────────────────────────────────

TONE_HZ = 800   # fixed tone frequency while object is detected


def _toggle(t):
    spk_pin.value(not spk_pin.value())


def speaker_on():
    spk_timer.init(freq=TONE_HZ * 2,
                   mode=machine.Timer.PERIODIC,
                   callback=_toggle)


def speaker_off():
    spk_timer.deinit()
    spk_pin.value(0)


# ── MAIN ──────────────────────────────────────────────────────

def main():
    print("Initialising MPU-6050...")
    mpu_init()
    print("Ready.\n")

    # Read initial accel as baseline for motion detection
    prev_ax, prev_ay, prev_az = mpu_read_accel()
    speaker_active = False

    while True:
        # ── MPU: detect motion ────────────────────────────────
        try:
            ax, ay, az = mpu_read_accel()
        except OSError as e:
            print("I2C error:", e)
            time.sleep_ms(200)
            continue

        delta = math.sqrt(
            (ax - prev_ax) ** 2 +
            (ay - prev_ay) ** 2 +
            (az - prev_az) ** 2
        )
        motion = delta > MOTION_THRESHOLD
        prev_ax, prev_ay, prev_az = ax, ay, az

        # ── HC-SR04: detect object ────────────────────────────
        dist    = measure_cm()
        object_detected = dist is not None and dist < DETECT_RANGE_CM

        # ── Speaker logic ─────────────────────────────────────
        # Sound ON  → object detected AND board is still
        # Sound OFF → motion detected OR nothing in range
        should_play = object_detected and not motion

        if should_play and not speaker_active:
            speaker_on()
            speaker_active = True
        elif not should_play and speaker_active:
            speaker_off()
            speaker_active = False

        # ── Debug output ──────────────────────────────────────
        d_str  = f"{dist:.1f} cm" if dist is not None else "---"
        state  = "PLAYING" if speaker_active else "SILENT "
        reason = ""
        if not object_detected:
            reason = "(no object)"
        elif motion:
            reason = "(motion detected — muted)"
        print(f"Dist: {d_str:8}  Motion: {'YES' if motion else 'no '}  Speaker: {state} {reason}")

        time.sleep_ms(100)


if __name__ == '__main__':
    main()
