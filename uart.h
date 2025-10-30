/*
 * uart.h
 *
 *  Created on: Nov 14, 2022
 *      Author: dans
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include "stm32h7xx_hal.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <lwrb.h>


typedef struct
{
	UART_HandleTypeDef *phuart;

	uint32_t dma_buffer_size;
	uint8_t *pDMABuffer;
	
	lwrb_t lwrb;
	uint32_t ring_buffer_size;
	uint8_t *pRingBuffer;
}myUART_t;



void setup_uart(myUART_t * myUARTHander, UART_HandleTypeDef * huart, uint32_t ring_buffer_size, uint32_t dma_buffer_size);

uint32_t bytes_in_buffer(myUART_t * myUARTHander);
int read_buffer_until(myUART_t * myUARTHander, uint8_t terminator, uint8_t * res, uint32_t res_size);

void send_uart(myUART_t * myUARTHander, const char *format, ...);
int split_csv_string(const char *input, char result[][20], const char *delimiter);




#endif /* INC_UART_H_ */
