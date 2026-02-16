/**
 * ChipWhisperer Timing Profiling Target Firmware
 * * This firmware is designed to measure baseline execution times on a target MCU.
 * It currently listens for a command ('m') with a 2-byte payload indicating the 
 * number of iterations to run a delay loop. The execution time is framed by hardware 
 * triggers (trigger_high / trigger_low) for precise oscilloscope measurement.
 */

#include "hal.h"
#include "simpleserial.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Generates a normally distributed (Gaussian) random number using the Irwin-Hall distribution.
 * * Note on PRNG: The standard C rand() is not cryptographically secure and is highly 
 * deterministic unless seeded with true entropy (e.g., a hardware TRNG) using srand().
 * * @param mean The desired average.
 * @param stddev The desired standard deviation.
 * @return A random float sampled from N(mean, stddev^2).
 */
// Reference: https://c-faq.com/lib/gaussian.html
float normal(float mean, float stddev) {
  float x = 0.0f;

  // Summing 12 uniform random variables approximates a normal distribution 
  // with a variance of 1, according to the Central Limit Theorem.
  for (int i = 0; i < 25; i++) {
    x += (float)rand() / (float)RAND_MAX;
  }

  x -= 12.5f; 

  x /= sqrt(25.0f / 12.0f); 

  return (x * stddev) + mean;
}

/**
 * Target function executed when the ChipWhisperer sends the 'm' command.
 * Measures the execution time of a highly controlled NOP loop.
 * * @param pt Pointer to the payload byte array sent over SimpleSerial.
 * @param len Length of the payload.
 * @return 0x00 on success.
 */
// ============================================================================
// Command 'l': Loop Profiler
// ============================================================================

uint8_t run_loop_measurement(uint8_t *pt, uint8_t len) {
  uint16_t iters;
  memcpy(&iters, pt, 2);

  // START TIMING
  trigger_high();

  // Execute requested number of NOPs
  for (uint16_t i = 0; i < iters; i++) {
    asm volatile("nop");
  }

  // END TIMING
  trigger_low();

  // Acknowledge completion
  simpleserial_put('r', 1, (uint8_t *)"A"); 
  return 0x00;
}


// ============================================================================
// Command 'g': PRNG Generator Profiler
// ============================================================================

uint8_t run_prng_measurement(uint8_t *pt, uint8_t len) {
  // START TIMING
  trigger_high();

  // Execute the target PRNG function.
  // We mark the output as 'volatile' so the compiler doesn't completely delete 
  // the function call realizing the result is never mathematically used.
  volatile float dummy_result = normal(370.0f, 285.0f);

  // END TIMING
  trigger_low();

  // Acknowledge completion
  simpleserial_put('r', 1, (uint8_t *)"A"); 
  return 0x00;
}

/**
 * Main execution loop for the embedded target.
 */
int main(void) {
  platform_init();
  init_uart();
  trigger_setup();
  simpleserial_init();

  // Register the two distinct profiling commands.
  // 'l' expects 2 bytes (the uint16_t iteration count).
  // 'g' expects 0 bytes (no input needed, just run the function).
  simpleserial_addcmd('l', 2, run_loop_measurement);
  simpleserial_addcmd('g', 0, run_prng_measurement);

  while (1) {
    simpleserial_get();
  }
}