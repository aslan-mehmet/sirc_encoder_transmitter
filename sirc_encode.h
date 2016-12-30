#ifndef __SIRC_ENCODE_H
#define __SIRC_ENCODE_H

#include <stdint.h>

/* before unlocking encoding lock
 * gives receiver time to process the received frame
 * in us and uint16
 */
#define FRAME_END_DELAY 25000

/* initiliaze hardware */
void sirc_encode_init(void);

/* success returns 0
 * if another frame being send error 
 */
int8_t sirc_encode(uint8_t cmd, uint8_t addr); 

#endif
