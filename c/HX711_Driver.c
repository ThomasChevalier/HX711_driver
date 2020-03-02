#include "HX711_Driver.h"

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

#include <avr/io.h>
#include <avr/interrupt.h>

static uint32_t circBuffer[HX711_BUFFER_SIZE];
static uint8_t circBufferPos;
static uint8_t circBufferSize;

static uint8_t listening;

static uint32_t lost;
static uint8_t gain;
static uint8_t lastGain;

static uint32_t HX711_read_data(void);

void HX711_init(void)
{
	circBufferPos = circBufferSize = listening = lost = 0;
	gain = lastGain = HX711_A_GAIN_128;
	// DOUT in input mode
	HX711_DOUT_DDR &= ~(1<<HX711_DOUT_PIN_NUM);
	// SCK in output mode
	HX711_SCK_DDR &= ~(1<<HX711_SCK_PIN_NUM);
	HX711_power_up();
}

uint8_t HX711_availables()
{
	return circBufferSize;
}

uint32_t HX711_read(void)
{
	if(circBufferSize == 0){
		if(HX711_DOUT_PIN & (1<<HX711_DOUT_PIN_NUM)){
			return HX711_read_data();
		}
	}
	uint32_t val = circBuffer[0];
	circBufferSize -= 1;
	circBufferPos += 1;

	if(circBufferPos == HX711_BUFFER_SIZE){
		circBufferPos = 0;
	}

	return val;
}

void HX711_enable(void)
{
	// Enable pin change interrupt
	PCICR |= (1<<PCIE0);
	// Enable the pin interrupt
	PCMSK0 |= (1 << HX711_DOUT_PIN_CHANGE_NUM);
	listening = 1;
}

void HX711_disable(void)
{
	// Disable the pin interrupt
	PCMSK0 &= ~(1 << HX711_DOUT_PIN_CHANGE_NUM);
	listening = 0;
}

uint32_t HX711_lost(void)
{
	return lost;
}

void HX711_reset_lost(void)
{
	lost = 0;
}

void HX711_set_gain(uint8_t gain_)
{
	if(gain_ - 1 < 3){	
		gain = gain_;
	}
}

void HX711_power_down(void)
{
	// SCK up for 60 µs at least
	HX711_SCK_PORT |= (1<<HX711_SCK_PIN_NUM);
}

void HX711_power_up(void)
{
	// The chip was powered down, default gain is HX711_A_GAIN_128
	if(HX711_SCK_PORT & (1<<HX711_SCK_PIN_NUM)){
		lastGain = HX711_A_GAIN_128;
	}
	// SCK low
	HX711_SCK_PORT &= ~(1<<HX711_SCK_PIN_NUM);
}

// /////// //
// PRIVATE //
// /////// //

uint32_t HX711_read_data(void)
{
	uint32_t val = 0;

	for(uint8_t i = 0; i < 24; ++i){
		HX711_SCK_PORT |= (1<<HX711_SCK_PIN_NUM);

		// The DOUT pin will be ready 0.1µs max after the rising of SCK
		#if F_CPU <= 20000000 && F_CPU > 10000000// instruction take 0.05µs or longer (max 0.1µs)
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		#else
		// Do not wait more than necessary
		__asm__ __volatile__ ("nop");
		#endif

		// Note : the SCK pin should not be in high state more than 50µs

		uint8_t dout_val = HX711_DOUT_PIN; // Just read the pin port, calculation is performed after

		HX711_SCK_PORT &= ~(1<<HX711_SCK_PIN_NUM);

		// There is no maximum time in low state, so we perform the calculation after the pin has been
		// set to low.
		// More over, the sck pin must stay at least 0.2µs in low state, the delay is here built with the 
		// calculation
		dout_val = (dout_val >> HX711_DOUT_PIN_NUM) & 0xFF;
		val <<= 1;
		val |= dout_val;
	}
	for(uint8_t i = 0; i < gain; ++i){
		HX711_SCK_PORT |= (1<<HX711_SCK_PIN_NUM);

		// Same delay as above
		#if F_CPU <= 20000000 && F_CPU > 10000000
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		#else
		__asm__ __volatile__ ("nop");
		#endif

		HX711_SCK_PORT &= ~(1<<HX711_SCK_PIN_NUM);

		// Do not forget the delay with the jump 
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
	}

	// Mark the value with the corresponding gain
	// Only 24 bit of the 32 bit value are used by the data
	// So the MSB and the 31th bit are used to code the channel
	// the data has been read from
	//
	// The gain is not changed immediately, we have to use the last gain requested
	val |= (uint32_t)lastGain << 30;
	// Update the gain
	lastGain = gain;

	// Write the value in the circular buffer
	if(listening){
		circBuffer[(circBufferPos + circBufferSize) % 8] = val;

		if(circBufferSize != HX711_BUFFER_SIZE){
			++circBufferSize;
		}
		else{
			++circBufferPos;
			if(circBufferPos == HX711_BUFFER_SIZE){
				circBufferPos = 0;
			}

			++lost;
			// Detect overflow
			if(lost == 0){
				lost = 0xFFFFFFFF;
			}
		}
	}
	return val;
}

ISR(PCINT0_vect)
{
	// Are data available ?
	if(HX711_DOUT_PIN & (1<<HX711_DOUT_PIN_NUM)){
		HX711_read_data();
	}
}
