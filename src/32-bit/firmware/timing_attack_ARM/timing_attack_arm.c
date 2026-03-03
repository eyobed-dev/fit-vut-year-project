/**
 * Timing Attack Target Firmware & Countermeasure (ARM Cortex-M4 / STM32F3)
 * * This firmware acts as the target for the ChipWhisperer timing side-channel
 * analysis. It receives a float input and a function choice via a serial
 * protocol, computes an activation function (ReLU, Sigmoid, or Tanh), and
 * returns the result.
 * * VULNERABILITY:
 * The conditional branches (if/else) and the varying mathematical complexity of
 * the activation functions cause the execution time to vary based on the input
 * and choice.
 * * COUNTERMEASURE:
 * To mask the actual execution time and protect against timing attacks, a
 * "desynchronization" technique is used. We inject a randomized number of dummy
 * instructions (NOPs) drawn from a Gaussian distribution to pad the execution
 * time, making it impossible for an attacker to correlate execution time with
 * the specific execution path.
 */

#include "hal.h"
#include "simpleserial.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Generates a normally distributed (Gaussian) random number.
 * * Embedded systems often lack hardware support for complex random
 * distributions. This function uses the Irwin-Hall distribution based on the
 * Central Limit Theorem. By summing 12 uniformly distributed random numbers
 * between 0 and 1, we approximate a standard normal distribution (mean = 0,
 * variance = 1).
 * * @param mean The desired average delay.
 * @param stddev The desired standard deviation (spread) of the delay.
 * @return A random float sampled from N(mean, stddev^2).
 */
// Reference: https://c-faq.com/lib/gaussian.html
float normal(float mean, float stddev) {

  float x = 0.0f;

  for (int i = 0; i < 25; i++) {
    x += (float)rand() / (float)RAND_MAX;
  }

  x -= 12.5f;

  x /= sqrt(25.0f / 12.0f);

  return (x * stddev) + mean;
}

/**
 * The primary target function executed when the ChipWhisperer sends a 'p'
 * command.
 * * @param pt Pointer to the payload byte array sent over SimpleSerial.
 * @param len Length of the payload.
 * @return 0x00 on success.
 */
uint8_t run_activation(uint8_t *pt, uint8_t len) {

  uint8_t choice = pt[0]; // 0 = ReLU, 1 = Sigmoid, 2 = Tanh

  float input;
  float output;

  memcpy(&input, pt + 1, 4);

  // --- ADD DESYNCHRONIZATION (RANDOM DELAY) COUNTERMEASURE ---
  // Target Mean = (Cluster difference - setup overhead) / Time per Loop
  // Iteration Target Mean = (287.88us - 2.03us) / 1.2223us = ~233.8 iterations
  // Target StdDev = (124.72us - 2.03us) / 1.2223us = ~100.3 iterations

  float raw_delay = normal(2339, 250);

  if (raw_delay < 0) {
    raw_delay = 0;
  }
  uint16_t delay_iters = (uint16_t)raw_delay;

  // --- START MEASUREMENT ---
  trigger_high();

  if (choice == 0) {
    // ReLU
    output = (input <= 0.0f) ? 0.0f : input;
    delay_iters += 233;
  } else if (choice == 1) {
    // Sigmoid
    output = 1.0f / (1.0f + expf(-input));
    delay_iters += 70;
  } else {
    // Tanh
    output = tanhf(input);
  }

  for (uint16_t i = 0; i < delay_iters; i++) {
    asm volatile("nop");
  }

  // --- END MEASUREMENT ---
  trigger_low();

  simpleserial_put('r', 4, (uint8_t *)&output);

  return 0x00;
}

int main(void) {
  platform_init();
  init_uart();
  trigger_setup();
  simpleserial_init();

  simpleserial_addcmd('p', 5, run_activation);

  while (1) {
    simpleserial_get();
  }
}