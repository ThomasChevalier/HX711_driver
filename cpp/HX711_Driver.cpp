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

static HX711_Driver* _driver = nullptr;

HX711_Driver::HX711_Driver():
	m_circBufferPos(0),
	m_circBufferSize(0),
	m_listening(false),
	m_lost(0),
	m_gain(HX711_A_GAIN_128),
	m_lastGain(HX711_A_GAIN_128)
{
	// DOUT in input mode
	HX711_DOUT_DDR &= ~(1<<HX711_DOUT_PIN_NUM);
	// SCK in output mode
	HX711_SCK_DDR &= ~(1<<HX711_SCK_PIN_NUM);
	power_up();

	// Claim
	_driver = this;
}

uint8_t HX711_Driver::availables() const
{
	return m_circBufferSize;
}

uint32_t HX711_Driver::read()
{
	if(m_circBufferSize == 0){
		if(HX711_DOUT_PIN & (1<<HX711_DOUT_PIN_NUM)){
			return read_data();
		}
	}
	uint32_t val = m_circBuffer[0];
	m_circBufferSize -= 1;
	m_circBufferPos += 1;

	if(m_circBufferPos == HX711_BUFFER_SIZE){
		m_circBufferPos = 0;
	}

	return val;
}

void HX711_Driver::enable()
{
	// Enable pin change interrupt
	PCICR |= (1<<PCIE0);

	// Enable the pin interrupt
	PCMSK0 |= (1 << HX711_DOUT_PIN_CHANGE_NUM);

	m_listening = true;
}

void HX711_Driver::disable()
{
	// Disable the pin interrupt
	PCMSK0 &= ~(1 << HX711_DOUT_PIN_CHANGE_NUM);
	m_listening = false;
}

uint32_t HX711_Driver::lost()
{
	return m_lost;
}

void HX711_Driver::reset_lost()
{
	m_lost = 0;
}

void HX711_Driver::set_gain(uint8_t gain)
{
	if(m_gain - 1 < 3){	
		m_gain = gain;
	}
}

void HX711_Driver::power_down()
{
	// SCK up for 60 µs at least
	HX711_SCK_PORT |= (1<<HX711_SCK_PIN_NUM);
}

void HX711_Driver::power_up()
{
	// The chip was powered down, default gain is HX711_A_GAIN_128
	if(HX711_SCK_PORT & (1<<HX711_SCK_PIN_NUM)){
		m_lastGain = HX711_A_GAIN_128;
	}
	// SCK low
	HX711_SCK_PORT &= ~(1<<HX711_SCK_PIN_NUM);
}

// /////// //
// PRIVATE //
// /////// //

uint32_t HX711_Driver::read_data()
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
	for(uint8_t i = 0; i < m_gain; ++i){
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
	val |= static_cast<uint32_t>(m_lastGain) << 30;
	// Update the gain
	m_lastGain = m_gain;

	// Write the value in the circular buffer
	if(m_listening){
		m_circBuffer[(m_circBufferPos + m_circBufferSize) % 8] = val;

		if(m_circBufferSize != HX711_BUFFER_SIZE){
			++m_circBufferSize;
		}
		else{
			++m_circBufferPos;
			if(m_circBufferPos == HX711_BUFFER_SIZE){
				m_circBufferPos = 0;
			}

			++m_lost;
			// Detect overflow
			if(m_lost == 0){
				m_lost = 0xFFFFFFFF;
			}
		}
	}
	return val;
}

ISR(PCINT0_vect)
{
	// Are data available ?
	if(HX711_DOUT_PIN & (1<<HX711_DOUT_PIN_NUM)){
		_driver->read_data();
	}
}
