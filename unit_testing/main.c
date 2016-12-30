#include "stm32f0xx.h"
#include "sirc_encode.h"

/* frame.pdf oscilloscope screenshot of single frame 
 * it is connected to led_driver transistor collector
 */
void main()
{
        sirc_encode_init();
	
        while(1)
                sirc_encode(21, 1);
}
