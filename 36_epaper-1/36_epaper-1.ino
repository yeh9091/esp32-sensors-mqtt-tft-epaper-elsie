/*
 * Waveshare 2.9-inch e-Paper (B) V4 / Rev2.1
 * ESP32 Dev Module
 *
 * This sketch is a clean image-only version of 36_epaper.
 * It displays the supplied Mid-Autumn Festival reference image once,
 * then puts the e-paper panel into sleep mode.
 *
 * Pin map:
 *   DIN  -> GPIO23
 *   CLK  -> GPIO18
 *   CS   -> GPIO27
 *   DC   -> GPIO26
 *   RST  -> GPIO25
 *   BUSY -> GPIO34
 */

#include <Arduino.h>
#include "epd2in9b_V4.h"
#include "reference_3color.h"

static constexpr uint16_t BUFFER_SIZE = (EPD_WIDTH / 8) * EPD_HEIGHT;
static uint8_t blackImage[BUFFER_SIZE];
static uint8_t redImage[BUFFER_SIZE];

Epd epd;

// The physical panel is 296 x 128, while the controller buffer is 128 x 296.
// This maps a physical pixel to the controller's logical coordinate system.
void setPixelPhysical(int x, int y, bool black, bool red) {
  if (x < 0 || x >= REFERENCE_IMAGE_W ||
      y < 0 || y >= REFERENCE_IMAGE_H) {
    return;
  }

  const int logicalX = y;
  const int logicalY = EPD_HEIGHT - 1 - x;
  const uint16_t index = logicalY * (EPD_WIDTH / 8) + logicalX / 8;
  const uint8_t mask = 0x80 >> (logicalX % 8);

  if (black) {
    blackImage[index] &= static_cast<uint8_t>(~mask);
  } else {
    blackImage[index] |= mask;
  }

  if (red) {
    redImage[index] &= static_cast<uint8_t>(~mask);
  } else {
    redImage[index] |= mask;
  }
}

void drawReferenceImage() {
  memset(blackImage, 0xFF, sizeof(blackImage));
  memset(redImage, 0xFF, sizeof(redImage));

  for (uint16_t y = 0; y < REFERENCE_IMAGE_H; ++y) {
    for (uint16_t x = 0; x < REFERENCE_IMAGE_W; ++x) {
      const uint32_t pixelIndex =
          static_cast<uint32_t>(y) * REFERENCE_IMAGE_W + x;
      const uint8_t packed =
          pgm_read_byte(&REFERENCE_IMAGE_3COLOR[pixelIndex / 4]);
      const uint8_t color =
          (packed >> ((3 - (pixelIndex % 4)) * 2)) & 0x03;

      if (color == 1) {
        setPixelPhysical(x, y, true, false);
      } else if (color == 2) {
        setPixelPhysical(x, y, false, true);
      }
      // color 0 is white and is already present after memset().
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("36_epaper-1: display reference image");

  if (epd.Init() != 0) {
    Serial.println("e-Paper init failed");
    return;
  }

  drawReferenceImage();
  Serial.println("Refreshing e-paper image...");
  epd.Display(blackImage, redImage);
  epd.Sleep();
  Serial.println("Display complete");
}

void loop() {
  // The image is static; keep the panel asleep after the first refresh.
  delay(1000);
}
