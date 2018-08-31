#include "../HX711_Driver.h"

int main(void)
{
	HX711_init();
	HX711_enable();

	while(1)
	{
		// Put the mcu in a sleep mode where it can be wake up by a
		// pin change interrupt
		//sleep();

		while(HX711_availables()){
			uint32_t raw = HX711_read();
			raw &= 0xC0000000;
			// Do wathever you want with the value...
		}
	}
}
