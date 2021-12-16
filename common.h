#ifndef _COMMON_H
#define _COMMON_H

/* COMMON.H
 * This header contains some common definitions used across the project.
 */

typedef uint32_t key_t;     // Represents a single key
typedef uint32_t keys_t;    // Represents a set of keys

// Lock out timer interrupt during spi comm to port expander
// This is necessary if you use the SPI2 channel in an ISR
#define start_spi2_critical_section INTEnable(INT_T2, 0);
#define end_spi2_critical_section INTEnable(INT_T2, 1);

// For debugging. Logs to the TFT at the current cursor position.
void tft_log(const char *format, ...);

#endif
