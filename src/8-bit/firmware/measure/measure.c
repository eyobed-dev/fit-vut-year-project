#include "hal.h"
#include "simpleserial.h"
#include <avr/interrupt.h>
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

  float x = 0;

  for (int i = 0; i < 4; i++) {
    x += (float)rand() / RAND_MAX;
  }

  x -= 4.0 / 2.0;
  x /= sqrt(4.0 / 12.0);

  return x * stddev + mean;
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

    cli(); 


    trigger_high();
    for (uint16_t i = 0; i < iters; i++) {
        asm volatile("nop"); 
    }
    trigger_low();


    sei();
    simpleserial_put('r', 1, (uint8_t *)"A"); // Acknowledge
   
    return 0x00;
}
// ============================================================================
// Command 'g': PRNG Generator Profiler
// ============================================================================

uint8_t run_prng_measurement(uint8_t *pt, uint8_t len) {

  cli();

  // START TIMING
  trigger_high();

  // Execute the target PRNG function.
  // We mark the output as 'volatile' so the compiler doesn't completely delete 
  // the function call realizing the result is never mathematically used.
  volatile float dummy_result = normal(370, 285);

  // END TIMING
  trigger_low();

  sei();
  simpleserial_put('r', 1, (uint8_t *)"A"); 
  return 0x00;
}

int main(void) {
  platform_init();
  init_uart();
  trigger_setup();
  simpleserial_init();

  // Listen for command
  simpleserial_addcmd('l', 2, run_loop_measurement);
  simpleserial_addcmd('g', 0, run_prng_measurement);

  while (1) {
    simpleserial_get();
  }
}
