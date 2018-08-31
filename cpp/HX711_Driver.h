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

// Needed to friend ISR(PCINT0)
extern "C" void __vector_3 (void); 
/**
 * @brief Interrupt based HX711 driver.
 * Based on the datasheet https://www.mouser.com/ds/2/813/hx711_english-1022875.pdf
 */
class HX711_Driver
{
public:
	HX711_Driver();

	/**
	 * @brief Number of measure currently available.
	 */
	uint8_t availables() const;

	/**
	 * @brief Read one raw measure, FIFO.
	 * Take care to remove the MSB and the 31th bit before using the value.
	 * The 32th and 31th bit describe the channel and gain used, compare them
	 * to HX711_*_GAIN_* to know the value.
	 */
	uint32_t read();

	/**
	 * @brief Enable the interruption handler.
	 */
	void enable();

	/**
	 * @brief Disable the interruption handler.
	 */
	void disable();

	/**
	 * @brief Number of measure lost because the circular buffer has not been read.
	 */
	uint32_t lost();
	void reset_lost();

	/**
	 * @brief Set gain and channel read for the next measure
	 * @details gain must be set with HX711_*_GAIN_* macro
	 * 
	 * @param gain HX711_*_GAIN_* macro
	 */
	void set_gain(uint8_t gain);

	void power_down();
	void power_up();

private:
	uint32_t m_circBuffer[HX711_BUFFER_SIZE];
	uint8_t m_circBufferPos;
	uint8_t m_circBufferSize;

	bool m_listening;

	uint32_t m_lost;
	uint8_t m_gain;
	uint8_t m_lastGain;

	uint32_t read_data();

	friend void __vector_3 (void); // == ISR(PCINT0) for the atmega328. You may need to change this.
};

#endif // HX711_DRIVER_HEADER_THOMAS_CHEVALIER
