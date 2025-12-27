/**
 * Timing Attack Target Firmware (ChipWhisperer Target)
 * SimpleSerial target executing activation functions.
 */

#include "hal.h"
#include "simpleserial.h"
#include <avr/interrupt.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint8_t run_activation(uint8_t *pt, uint8_t len) {
  uint8_t choice = pt[0]; // 0 = ReLU, 1 = Sigmoid, 2 = Tanh
  float input;
  float output;

  memcpy(&input, pt + 1, 4);

  // --- START MEASUREMENT ---
  trigger_high();

  if (choice == 0) {
    // ReLU
    output = (input <= 0.0f) ? 0.0f : input;
  } else if (choice == 1) {
    // Sigmoid
    output = 1.0f / (1.0f + expf(-input));
  } else {
    // Tanh
    output = tanhf(input);
  }

  trigger_low();
  // --- END MEASUREMENT ---

  simpleserial_put('r', 4, (uint8_t *)&output);

  return 0x00;
}

int main(void) {
  platform_init();
  init_uart();
  trigger_setup();
  simpleserial_init();

  // Listen for command
  simpleserial_addcmd('p', 5, run_activation);

  while (1) {
    simpleserial_get();
  }
}
