# HX711_driver
A HX711 driver for AVR microcontroller.

The driver is based on pin change interrupt, but you can also read data as usual, by not enabling the driver.
When data are read using the interrupt handler, they are stored in a circular buffer. If the buffer is full the the oldest data are removed.
When reading from this buffer (using 'read' method) the oldest data are returned first (FIFO).
