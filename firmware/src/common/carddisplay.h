// The 2.9" grayscale Wing, with the driver's fixed waits made adjustable.
//
// The Wing leaves the panel's BUSY line unconnected, so Adafruit_SSD1680 cannot
// tell when the panel is done and instead waits a fixed worst case: 500 ms at
// every reset and after every update, plus default_refresh_delay (9000 ms in
// grayscale) for the refresh itself. Measured on unit 2 (2026-09-22), one card
// cost 1000 ms in begin() and 1120 ms inside display() before a pixel moved.
//
// Both waits are settable here, and displayNoPowerUp() skips the panel
// re-initialisation for a redraw that follows an earlier display() - the panel
// is still powered, since display() only powers down when asked to.
#pragma once

#include "Adafruit_ThinkInk.h"

class CardDisplay : public ThinkInk_290_Grayscale4_FPC7519 {
 public:
  using ThinkInk_290_Grayscale4_FPC7519::ThinkInk_290_Grayscale4_FPC7519;

  // Wait after the refresh is started, in place of watching BUSY.
  void setRefreshDelay(uint16_t ms) { default_refresh_delay = ms; }

  // Wait wherever the driver polls BUSY (resets, end of update). The driver's
  // own value is 500 ms.
  void setBusyWait(uint16_t ms) { busyWaitMs = ms; }

  // display() without powerUp(): only valid when the panel was left powered by
  // an earlier display(), which is the case for a menu redraw.
  void displayNoPowerUp() {
    setRAMAddress(0, 0);
    writeRAMFramebufferToEPD(buffer1, buffer1_size, 0);
    if (buffer2_size != 0) {
      delay(2);
      setRAMAddress(0, 0);
      writeRAMFramebufferToEPD(buffer2, buffer2_size, 1);
    }
    update();
  }

 protected:
  void busy_wait(void) override { delay(busyWaitMs); }

 private:
  uint16_t busyWaitMs = 500;
};
