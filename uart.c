/*
 * uart.c
 *
 *  Created on: Nov 14, 2022
 *      Author: dans
 */

#include <uart.h>


//---------------------------------------------------------------------------
//--------------------------- GENERAL----------------------------------------
//---------------------------------------------------------------------------
//keep all the addresses of the instances of myuart for use in the callback
#define POINTER_ARRAY_SIZE 10
myUART_t * array_of_pointers[POINTER_ARRAY_SIZE] = {0};
uint8_t array_of_pointers_idx = 0;

//this DMA_rx_buffer will be used for all uarts, after receiving data should go in a ring buffer
#define DMA_RX_BUFFER_SIZE 100
uint8_t DMA_rx_buffer[DMA_RX_BUFFER_SIZE];

//this is the general callback fired at any uart transaction
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(Size == 0)
		return;

	HAL_UART_DMAStop(huart);

	for(int i = 0; i < POINTER_ARRAY_SIZE; i++)
	{
		if(0 == array_of_pointers[i])
			break;

		if(array_of_pointers[i]->phuart == huart)
		{
			lwrb_write(&array_of_pointers[i]->lwrb, DMA_rx_buffer, Size);
		}
	}

	HAL_UARTEx_ReceiveToIdle_DMA(huart, DMA_rx_buffer, DMA_RX_BUFFER_SIZE);
}
//---------------------------------------------------------------------------



//-----------------------------------------------------------------------
void setup_uart(myUART_t * myUARTHander, UART_HandleTypeDef * huart)
{
	myUARTHander->phuart = huart;

	HAL_UARTEx_ReceiveToIdle_DMA(huart, DMA_rx_buffer, DMA_RX_BUFFER_SIZE);

	/*
	__HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
	HAL_UART_Receive_DMA(huart, DMA_rx_buffer, DMA_RX_BUFFER_SIZE);	//start dma
	 */

	//receive interrupts
	__HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT | DMA_IT_TC);

	//transfer interrupts
//    __HAL_DMA_DISABLE_IT(phuart->hdmatx, DMA_IT_HT);
//    __HAL_DMA_ENABLE_IT(phuart->hdmatx, DMA_IT_TC);

	//--------------------- ring buffer
	lwrb_init(&myUARTHander->lwrb, myUARTHander->ring_buffer, sizeof(myUARTHander->ring_buffer)); // Initialize ring buffer

	//--------------------- save to array
	array_of_pointers[array_of_pointers_idx] = myUARTHander;
	array_of_pointers_idx++;
}

//-------------------------------------------------------------------------
uint8_t is_buffer_empty(myUART_t * myUARTHander)
{
	int total_items_in_buffer = lwrb_get_full(&myUARTHander->lwrb);
	return total_items_in_buffer? 0:1;
}

//returns number of bytes in result
int read_buffer_until(myUART_t * myUARTHander, uint8_t terminator, uint8_t * res)
{
	int total_items_in_buffer = lwrb_get_full(&myUARTHander->lwrb);
	if(total_items_in_buffer)
	{
		//if there's something in the buffer, peek inside the buffer
		uint8_t temp_buf[RING_BUFFER_SIZE] = {0};
		int peeked_number = lwrb_peek(&myUARTHander->lwrb, 0, temp_buf, total_items_in_buffer);

		//look for terminator character
		int i;
		for(i = 0; i < peeked_number; i++)
		{
			if(temp_buf[i] == terminator)
				break;
		}

		if(i == total_items_in_buffer)
			return 0;
		else
			memcpy(res, temp_buf, i+1);

		lwrb_skip(&myUARTHander->lwrb, i+1);
		return i + 1;
	}

	return 0;
}



void send_uart(myUART_t * myUARTHander, const char *format, ...)
{
	char buffer[256]; // Adjust the buffer size as needed
	va_list args;     // Declare a variable of type va_list

	va_start(args, format); // Initialize args to store all values after format
	int len = vsnprintf(buffer, sizeof(buffer), format, args); // Format the string with the arguments
	va_end(args); // Clean up the va_list variable

	HAL_UART_Transmit(myUARTHander->phuart, (uint8_t *) buffer, len, 0xFFFF);
}

//----------------------------------------------------------------------------



/*
void USER_UART_IRQHandler(UART_HandleTypeDef *huart)
{
	if(RESET != __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
	{
		__HAL_UART_CLEAR_IDLEFLAG(huart);
		HAL_UART_DMAStop(huart);

//			int new_data_length  = DMA_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
		int new_data_length  = DMA_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart->hdmarx);

		if(new_data_length > 0)
		{
//				lwrb_write(&buff, DMA_rx_buffer, new_data_length);
		}

		HAL_UART_Receive_DMA(myUart.phuart, myUart.DMA_rx_buffer, DMA_RX_BUFFER_SIZE);	//start dma

	}

}

	//TODO expand this to a more generic thing
//	if(huart == myUart.phuart)
//	{
//		lwrb_write(&myUart.lwrb, DMA_rx_buffer, Size);
//	}
*/


/*
void send_uart(char* packet)
{
	char p[64] = {0};

	snprintf(p, 64, "UART>> %s\r\n", packet);
	HAL_UART_Transmit(&huart1, (uint8_t *)&p, strlen(p), 0xFFFF);
}
*/


