#ifndef __SIRC_ENCODE_H
#define __SIRC_ENCODE_H

/* this function error prone some reset values assumed
 * when setting peripheral registers
 */
void sirc_encode_init(void);
void sirc_encode(uint8_t cmd, uint8_t addr);

#endif
