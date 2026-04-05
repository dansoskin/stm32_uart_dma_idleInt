


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
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include <lwrb.h>

#define UART_TX_BUFFER_SIZE 256U


typedef struct
{
	UART_HandleTypeDef *phuart;

	uint32_t dma_buffer_size;
	uint8_t *pDMABuffer;
	
	lwrb_t lwrb;
	uint32_t ring_buffer_size;
	uint8_t *pRingBuffer;

	volatile uint8_t tx_dma_busy;
	uint8_t tx_dma_buffer[UART_TX_BUFFER_SIZE];
}myUART_t;


bool setup_uart(myUART_t * myUARTHander,
		UART_HandleTypeDef * huart,
		uint8_t * ring_buffer,
		uint32_t ring_buffer_size,
		uint8_t * dma_buffer,
		uint32_t dma_buffer_size);

uint32_t bytes_in_buffer(myUART_t * myUARTHander);
int read_buffer_until(myUART_t * myUARTHander, uint8_t terminator, uint8_t * res, uint32_t res_size);

void send_uart(myUART_t * myUARTHander, const char *format, ...);
HAL_StatusTypeDef send_uart_dma(myUART_t * myUARTHander, const char *format, ...);
int split_csv_string(const char *input, char result[][20], const char *delimiter);




#endif /* INC_UART_H_ */
