# Hand Motion Detector

NUCLEO-G474RE + MPU-6050 (GY-521) + 2 LEDs

## Hardware
- **Board:** ST NUCLEO-G474RE
- **IMU:** GY-521 (MPU-6050) — breadboard rows 25–30
- **Blue LED:** D3 (PB_3) — triggers on LEFT / UP tilt
- **Orange LED:** D12 (PA_6) — triggers on RIGHT / DOWN tilt

## Wiring
| GY-521 Pin | Row | Connected to |
|---|---|---|
| VCC | 25 | + power rail (5V) |
| GND | 26 | − power rail (GND) |
| SCL | 27 | D15 (PB_8) |
| SDA | 28 | D14 (PB_9) |

Power rail + → NUCLEO E5V  
Power rail − → NUCLEO GND

## Build
```bash
mbed-tools compile -m NUCLEO_G474RE -t GCC_ARM --flash
