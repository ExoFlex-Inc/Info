/*
 * uartRingBufDMA.h
 *
 *  Created on: Aug 12, 2021
 *      Author: controllerstech.com
 */

#ifndef INC_UARTRINGBUFDMA_H_
#define INC_UARTRINGBUFDMA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#include <stdint.h>

/* Initialize the Ring buffer
 * It will also initialize the UART RECEIVE DMA
 * */
void Ringbuf_Init (void);

/* Reset the ring buffer
 * Resets the Head and Tail, also the buffers
 * */
void Ringbuf_Reset (UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* INC_UARTRINGBUFDMA_H_ */
