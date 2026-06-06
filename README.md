# Hand Motion + Distance Detector

NUCLEO-G474RE + MPU-6050 (GY-521) + HC-SR04 + 2 LEDs

## Hardware
- **Board:** ST NUCLEO-G474RE
- **IMU:** GY-521 (MPU-6050) — breadboard rows 25–30
- **Ultrasonic:** HC-SR04 — direct wires to NUCLEO
- **Blue LED:** D3 (PB_3) — breadboard row 3 anode
- **Orange LED:** D6 (PB_10) — breadboard row 12 anode

## Wiring

### Power rails
| Breadboard | NUCLEO |
|---|---|
| + rail (row 17) | E5V |
| − rail (row 17) | GND |

### MPU-6050 (GY-521) — breadboard rows 25–30
| GY-521 Pin | Row | NUCLEO |
|---|---|---|
| VCC | 25 | + rail |
| GND | 26 | − rail |
| SCL | 27 | D15 (PB_8) |
| SDA | 28 | D14 (PB_9) |

### HC-SR04 — direct to NUCLEO
| HC-SR04 Pin | NUCLEO |
|---|---|
| VCC | + rail |
| GND | − rail |
| TRIG | D12 (PA_6) |
| ECHO | D11 (PA_7) |

### LEDs — breadboard
| LED | Anode row | Anode wire to NUCLEO | Cathode |
|---|---|---|---|
| Blue | row 3 | D3 (PB_3) | 220Ω → − rail |
| Orange | row 12 | D6 (PB_10) | 220Ω → − rail |

## Behaviour
| Condition | Blue LED | Orange LED |
|---|---|---|
| Tilt LEFT or UP | ON | off |
| Tilt RIGHT or DOWN | off | ON |
| Fast shake | flash | flash |
| Still + distance < 20cm | ON | off |
| Still + distance ≥ 20cm | off | ON |

## Build
```bash
mbed-tools compile -m NUCLEO_G474RE -t GCC_ARM --flash
```

## Serial Monitor
9600-8-N-1 on COM3
