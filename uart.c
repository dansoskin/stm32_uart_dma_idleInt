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


//this is the general callback fired at any uart transaction
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(Size == 0)
		return;

	for(int i = 0; i < POINTER_ARRAY_SIZE; i++)
	{
		if(0 == array_of_pointers[i])
			break;

		if(array_of_pointers[i]->phuart == huart)
		{
			HAL_UART_RxEventTypeTypeDef event_type = HAL_UARTEx_GetRxEventType(huart);
			if ((event_type == HAL_UART_RXEVENT_IDLE) && (Size >= array_of_pointers[i]->dma_buffer_size))
			{
				HAL_UART_DMAStop(huart);
				HAL_UARTEx_ReceiveToIdle_DMA(huart, array_of_pointers[i]->pDMABuffer, array_of_pointers[i]->dma_buffer_size);
				return;
			}

			HAL_UART_DMAStop(huart);
			lwrb_write(&array_of_pointers[i]->lwrb, array_of_pointers[i]->pDMABuffer, Size);
			HAL_UARTEx_ReceiveToIdle_DMA(huart, array_of_pointers[i]->pDMABuffer, array_of_pointers[i]->dma_buffer_size);
			return;
		}
	}

}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->ErrorCode & HAL_UART_ERROR_PE)
	{
	// Parity error occurred
	  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF);
	}
	if (huart->ErrorCode & HAL_UART_ERROR_FE)
	{
	// Frame error occurred
	  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF);
	}
	if(huart->ErrorCode & HAL_UART_ERROR_NE)
	{
	  // Noise error occurred
	  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_NEF);
	}
	if (huart->ErrorCode & HAL_UART_ERROR_ORE)
	{
	// Overrun error occurred
	  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF); // Clear Overrun Error Flag
	}
	if (huart->ErrorCode & HAL_UART_ERROR_DMA)
	{
	// DMA transfer error occurred
	}
	// Handle other error types as needed

	HAL_UART_DMAStop(huart);

	//restart the DMA
	for(int i = 0; i < POINTER_ARRAY_SIZE; i++)
	{
		if(0 == array_of_pointers[i])
			break;

		if(array_of_pointers[i]->phuart == huart)
		{
			HAL_UARTEx_ReceiveToIdle_DMA(huart, array_of_pointers[i]->pDMABuffer, array_of_pointers[i]->dma_buffer_size);
		}
	}

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	for(int i = 0; i < POINTER_ARRAY_SIZE; i++)
	{
		if(0 == array_of_pointers[i])
			break;

		if(array_of_pointers[i]->phuart == huart)
		{
			array_of_pointers[i]->tx_dma_busy = 0;
			break;
		}
	}
}
//---------------------------------------------------------------------------



//-----------------------------------------------------------------------
bool setup_uart(myUART_t * myUARTHander,
		UART_HandleTypeDef * huart,
		uint8_t * ring_buffer,
		uint32_t ring_buffer_size,
		uint8_t * dma_buffer,
		uint32_t dma_buffer_size)
{
	if ((myUARTHander == NULL) || (huart == NULL) || (ring_buffer == NULL) || (dma_buffer == NULL)
			|| (ring_buffer_size == 0U) || (dma_buffer_size == 0U) || (array_of_pointers_idx >= POINTER_ARRAY_SIZE))
	{
		return false;
	}

	myUARTHander->phuart = huart;
	myUARTHander->ring_buffer_size = ring_buffer_size;
	myUARTHander->dma_buffer_size = dma_buffer_size;
	myUARTHander->tx_dma_busy = 0;
	memset(myUARTHander->tx_dma_buffer, 0, sizeof(myUARTHander->tx_dma_buffer));
	
	myUARTHander->pRingBuffer = ring_buffer;
	myUARTHander->pDMABuffer = dma_buffer;
	memset(myUARTHander->pRingBuffer, 0 , ring_buffer_size);
	memset(myUARTHander->pDMABuffer, 0 , dma_buffer_size);

	//--------------------- ring buffer
	lwrb_init(&myUARTHander->lwrb, myUARTHander->pRingBuffer, myUARTHander->ring_buffer_size); // Initialize ring buffer

	//--------------------- save to array
	array_of_pointers[array_of_pointers_idx] = myUARTHander;
	array_of_pointers_idx++;

	__HAL_UART_CLEAR_OREFLAG(huart);
	__HAL_UART_CLEAR_IDLEFLAG(huart);
	__HAL_UART_FLUSH_DRREGISTER(huart);
	HAL_UARTEx_ReceiveToIdle_DMA(huart, myUARTHander->pDMABuffer, myUARTHander->dma_buffer_size);

	//receive interrupts
	__HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT | DMA_IT_TC);

	//transfer interrupts
//    __HAL_DMA_DISABLE_IT(phuart->hdmatx, DMA_IT_HT);
//    __HAL_DMA_ENABLE_IT(phuart->hdmatx, DMA_IT_TC);



	return true;
}

//-------------------------------------------------------------------------
uint32_t bytes_in_buffer(myUART_t * myUARTHander)
{
	int total_items_in_buffer = lwrb_get_full(&myUARTHander->lwrb);
	return total_items_in_buffer;
}

//returns number of bytes in result
int read_buffer_until(myUART_t * myUARTHander, uint8_t terminator, uint8_t * res, uint32_t res_size)
{
	if ((myUARTHander == NULL) || (res == NULL) || (res_size == 0U))
	{
		return 0;
	}

	size_t total_items_in_buffer = lwrb_get_full(&myUARTHander->lwrb);
	if(total_items_in_buffer > 0U)
	{
		size_t bytes_to_scan = total_items_in_buffer;
		if (bytes_to_scan > res_size)
		{
			bytes_to_scan = res_size;
		}

		size_t peeked_number = lwrb_peek(&myUARTHander->lwrb, 0, res, bytes_to_scan);

		for(size_t i = 0; i < peeked_number; i++)
		{
			if(res[i] == terminator)
			{
				size_t bytes_read = lwrb_read(&myUARTHander->lwrb, res, i + 1U);
				return (int)bytes_read;
			}
		}

		return 0;
	}

	return 0;
}



void send_uart(myUART_t * myUARTHander, const char *format, ...)
{
	char buffer[UART_TX_BUFFER_SIZE];
	va_list args;     // Declare a variable of type va_list

	va_start(args, format); // Initialize args to store all values after format
	int len = vsnprintf(buffer, sizeof(buffer), format, args); // Format the string with the arguments
	va_end(args); // Clean up the va_list variable

	if (len < 0)
	{
		return;
	}
	if ((size_t)len >= sizeof(buffer))
	{
		len = (int)(sizeof(buffer) - 1U);
	}

	HAL_UART_Transmit(myUARTHander->phuart, (uint8_t *) buffer, (uint16_t)len, 0xFFFF);
}

HAL_StatusTypeDef send_uart_dma(myUART_t * myUARTHander, const char *format, ...)
{
	if ((myUARTHander == NULL) || (myUARTHander->phuart == NULL) || (format == NULL))
	{
		return HAL_ERROR;
	}

	va_list args;

	va_start(args, format);
	int len = vsnprintf((char *)myUARTHander->tx_dma_buffer, sizeof(myUARTHander->tx_dma_buffer), format, args);
	va_end(args);

	if (len < 0)
	{
		return HAL_ERROR;
	}
	if ((size_t)len >= sizeof(myUARTHander->tx_dma_buffer))
	{
		len = (int)(sizeof(myUARTHander->tx_dma_buffer) - 1U);
	}

	uint32_t start_tick = HAL_GetTick();
	while(myUARTHander->tx_dma_busy)
	{
		if ((HAL_GetTick() - start_tick) >= 1000U)
		{
			return HAL_TIMEOUT;
		}
	}

	myUARTHander->tx_dma_busy = 1;
	if (HAL_UART_Transmit_DMA(myUARTHander->phuart, myUARTHander->tx_dma_buffer, (uint16_t)len) != HAL_OK)
	{
		myUARTHander->tx_dma_busy = 0;
		return HAL_ERROR;
	}

	return HAL_OK;
}

//----------------------------------------------------------------------------

/*
 * usage example:
 * char result[10][20] = {0};	//10 values of 20 characters
 * int count = split_csv_string((char*)input, result, ",\n");
 */

int split_csv_string(const char *input, char result[][20], const char *delimiter)
{
    // need to use a copy of the input to avoid modifying the original string
    if (input == NULL || result == NULL || delimiter == NULL) {
        return 0; // Invalid input
    }

    char *input_copy = strdup(input);
    int count = 0;

    char * token = strtok(input_copy, delimiter);

    while (token != NULL && count < 10)
    {
        strncpy(result[count], token, 19);
        result[count][19] = '\0'; // Ensure null-termination
        count++;
        token = strtok(NULL, delimiter);
    }

    free(input_copy);
    return count;
}



