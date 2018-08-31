#ifndef HX711_DRIVER_HEADER_THOMAS_CHEVALIER
#define HX711_DRIVER_HEADER_THOMAS_CHEVALIER

/*
MIT License

Copyright (c) 2018 Thomas Chevalier

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "stdint.h"

#include <avr/interrupt.h>


// Options

// Currently the dout pin must be on the PCINT0 interrupt
#define HX711_DOUT_PIN_NUM 0
#define HX711_DOUT_DDR DDRB
#define HX711_DOUT_PORT PORTB
#define HX711_DOUT_PIN PINB
#define HX711_DOUT_PIN_CHANGE_NUM 1

#define HX711_SCK_PIN_NUM 0
#define HX711_SCK_DDR DDRB
#define HX711_SCK_PORT PORTB
#define HX711_SCK_PIN PINB

#define HX711_BUFFER_SIZE 8

// Constant

#define HX711_A_GAIN_128 1
#define HX711_B_GAIN_32  2
#define HX711_A_GAIN_64  3

/**
 * Interrupt based HX711 driver.
 * Based on the datasheet https://www.mouser.com/ds/2/813/hx711_english-1022875.pdf
 */


void HX711_init(void);

/**
 * @brief Number of measure currently available.
 */
uint8_t HX711_availables(void);

/**
 * @brief Read one raw measure, FIFO.
 * Take care to remove the MSB and the 31th bit before using the value.
 * The 32th and 31th bit describe the channel and gain used, compare them
 * to HX711_*_GAIN_* to know the value.
 */
uint32_t HX711_read(void);

/**
 * @brief Enable the interruption handler.
 */
void HX711_enable(void);

/**
 * @brief Disable the interruption handler.
 */
void HX711_disable(void);

/**
 * @brief Number of measure lost because the circular buffer has not been read.
 */
uint32_t HX711_lost(void);
void HX711_reset_lost(void);

/**
 * @brief Set gain and channel read for the next measure
 * @details gain must be set with HX711_*_GAIN_* macro
 * 
 * @param gain HX711_*_GAIN_* macro
 */
void HX711_set_gain(uint8_t gain_);

void HX711_power_down(void);
void HX711_power_up(void);


#endif // HX711_DRIVER_HEADER_THOMAS_CHEVALIER
