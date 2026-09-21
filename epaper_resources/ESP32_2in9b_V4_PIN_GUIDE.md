# Waveshare 2.9-inch e-Paper (B) V4 / Rev2.1

## Hardware identification

This guide assumes the module is the Waveshare **2.9-inch e-Paper (B) V4**:

- Resolution: 296 x 128
- Colors: black, white, red
- Interface: 4-wire SPI
- Driver board: Rev2.1 or newer

The Rev2.1 driver board includes level processing and accepts 3.3 V or 5 V
operation. For an ESP32, use **3.3 V** for VCC so that the power and signal
levels remain consistent.

## Recommended connection to the existing ESP32 project

The ILI9225 TFT will be removed from the project. The e-paper can therefore
use the class-standard control pins GPIO27 and GPIO26 while retaining the
ESP32 hardware SPI clock/data pins.

| e-Paper pin | ESP32 Dev Module | Purpose |
|---|---:|---|
| VCC | 3V3 | 3.3 V power |
| GND | GND | Ground |
| DIN | GPIO23 | SPI MOSI |
| CLK | GPIO18 | SPI SCK |
| CS | GPIO27 | E-paper chip select |
| DC | GPIO26 | Data/command |
| RST | GPIO25 | Hardware reset |
| BUSY | GPIO34 | Busy status input |

### Existing project pins that remain unchanged

| Function | GPIO |
|---|---:|
| DHT11 | 14 |
| Light sensor ADC | 33 |
| OLED SDA / SCL | 21 / 22 |
| Page button | 0 |
| Green / yellow / red LED | 15 / 2 / 4 |

## Important wiring notes

1. `BUSY` is connected to GPIO34, which is input-only on the classic ESP32.
   That is suitable because BUSY is an output from the e-paper module.
2. GPIO34 has no internal pull-up/pull-down. Use the module's BUSY signal
   directly; do not configure it with `INPUT_PULLUP`.
3. Do not connect the e-paper `BUSY` line to an ESP32 output.
4. Do not use the Arduino UNO defaults from the downloaded Waveshare demo
   (`RST=8`, `DC=9`, `CS=10`, `BUSY=7`) without changing them for ESP32.
5. The e-paper is slow to refresh. A full refresh can take roughly 12–15
   seconds and normal flashing during refresh is expected. Do not refresh it
   every second.
6. Disconnect power before inserting or removing the e-paper cable. Avoid
   bending or pressing the FPC cable/panel.

## Downloaded resources

The official Waveshare V4 Arduino demo source is in:

`epaper_resources/Waveshare_2in9b_V4_Arduino/`

The official V4 specification/manual is:

`epaper_resources/2.9inch-e-Paper-B-V4-user-manual.pdf`

The downloaded Arduino demo still contains Arduino UNO pin defaults. It is
reference code only until `epdif.h`/`epdif.cpp` are adapted to the ESP32 pin
map above. The original ILI9225 TFT pin definitions are no longer relevant
because that display is being removed.
