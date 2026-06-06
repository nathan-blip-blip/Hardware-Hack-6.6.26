#ifndef MPU6050_H
#define MPU6050_H

#include "mbed.h"

// MPU-6050 I2C address (AD0 pin tied to GND = 0x68, shifted left for Mbed = 0xD0)
#define MPU6050_ADDR        0xD0

// Register addresses (from datasheet PS-MPU-6000A-00 Rev 3.4)
#define REG_PWR_MGMT_1      0x6B
#define REG_SMPLRT_DIV      0x19
#define REG_CONFIG          0x1A
#define REG_GYRO_CONFIG     0x1B
#define REG_ACCEL_CONFIG    0x1C
#define REG_ACCEL_XOUT_H    0x3B   // Accel X high byte (followed by XL, YH, YL, ZH, ZL)
#define REG_GYRO_XOUT_H     0x43   // Gyro  X high byte (followed by XL, YH, YL, ZH, ZL)
#define REG_WHO_AM_I        0x75   // Should read 0x68

// Accel full-scale ±2g → sensitivity = 16384 LSB/g
#define ACCEL_SENSITIVITY   16384.0f
// Gyro full-scale ±250°/s → sensitivity = 131 LSB/(°/s)
#define GYRO_SENSITIVITY    131.0f

class MPU6050 {
public:
    MPU6050(PinName sda, PinName scl) : _i2c(sda, scl) {
        _i2c.frequency(400000); // 400 kHz Fast Mode
    }

    // Returns true if sensor is found and wakes up correctly
    bool init() {
        // Wake up: clear SLEEP bit in PWR_MGMT_1, use internal 8MHz oscillator
        char data[2] = { REG_PWR_MGMT_1, 0x00 };
        if (_i2c.write(MPU6050_ADDR, data, 2) != 0) return false;

        // Sample rate divider: SMPLRT_DIV=7 → 1kHz/(7+1) = 125 Hz
        data[0] = REG_SMPLRT_DIV; data[1] = 0x07;
        _i2c.write(MPU6050_ADDR, data, 2);

        // DLPF config: bandwidth ~44Hz (smooth out vibration noise)
        data[0] = REG_CONFIG; data[1] = 0x03;
        _i2c.write(MPU6050_ADDR, data, 2);

        // Gyro full scale ±250°/s
        data[0] = REG_GYRO_CONFIG; data[1] = 0x00;
        _i2c.write(MPU6050_ADDR, data, 2);

        // Accel full scale ±2g
        data[0] = REG_ACCEL_CONFIG; data[1] = 0x00;
        _i2c.write(MPU6050_ADDR, data, 2);

        return true;
    }

    // Read raw 16-bit signed values for accel (X,Y,Z) and gyro (X,Y,Z)
    void read_raw(int16_t &ax, int16_t &ay, int16_t &az,
                  int16_t &gx, int16_t &gy, int16_t &gz) {
        char reg = REG_ACCEL_XOUT_H;
        char buf[14];
        _i2c.write(MPU6050_ADDR, &reg, 1, true); // repeated start
        _i2c.read(MPU6050_ADDR, buf, 14);

        ax = (int16_t)((buf[0]  << 8) | buf[1]);
        ay = (int16_t)((buf[2]  << 8) | buf[3]);
        az = (int16_t)((buf[4]  << 8) | buf[5]);
        // buf[6..7] = temperature (skip)
        gx = (int16_t)((buf[8]  << 8) | buf[9]);
        gy = (int16_t)((buf[10] << 8) | buf[11]);
        gz = (int16_t)((buf[12] << 8) | buf[13]);
    }

    // Read accel in g-force units
    void read_accel_g(float &ax, float &ay, float &az) {
        int16_t raw_ax, raw_ay, raw_az, gx, gy, gz;
        read_raw(raw_ax, raw_ay, raw_az, gx, gy, gz);
        ax = raw_ax / ACCEL_SENSITIVITY;
        ay = raw_ay / ACCEL_SENSITIVITY;
        az = raw_az / ACCEL_SENSITIVITY;
    }

private:
    I2C _i2c;
};

#endif // MPU6050_H
