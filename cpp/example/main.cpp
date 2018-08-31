#include "../HX711_Driver.h"

int main(void)
{
	HX711_Driver hx;
	hx.enable();

	while(1)
	{
		// Put the mcu in a sleep mode where it can be wake up by a
		// pin change interrupt
		//sleep();

		while(hx.availables()){
			uint32_t raw = hx.read();
			raw &= 0xC0000000;
			// Do wathever you want with the value...
		}
	}
}
