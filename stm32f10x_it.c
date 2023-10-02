/*
 * stm32f1xx_it.c
 *
 *  Created on: Jul 12, 2016
 *  Autor:
 */


#include "stm32f10x_it.h"

#include "FreeRTOS.h"
#include "semphr.h" //added
#include "task.h"



/*******************************************************************************
* Function Name  : NMIException
* Description    : This function handles NMI exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NMIException(void)
{
}

/*******************************************************************************
* Function Name  : HardFaultException
* Description    : This function handles Hard Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void HardFaultException(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : MemManageException
* Description    : This function handles Memory Manage exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void MemManageException(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : BusFaultException
* Description    : This function handles Bus Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void BusFaultException(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : UsageFaultException
* Description    : This function handles Usage Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UsageFaultException(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : DebugMonitor
* Description    : This function handles Debug Monitor exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DebugMonitor(void)
{
}

/*******************************************************************************
* Function Name  : SVCHandler
* Description    : This function handles SVCall exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVCHandler(void)
{
}

/*******************************************************************************
* Function Name  : PendSVC
* Description    : This function handles PendSVC exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void PendSVC(void)
{
}

/*******************************************************************************
* Function Name  : SysTickHandler
* Description    : This function handles SysTick Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SysTickHandler(void)
{
}

/*******************************************************************************
* Function Name  : WWDG_IRQHandler
* Description    : This function handles WWDG interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void WWDG_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : PVD_IRQHandler
* Description    : This function handles PVD interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void PVD_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TAMPER_IRQHandler
* Description    : This function handles Tamper interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TAMPER_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : RTC_IRQHandler
* Description    : This function handles RTC global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RTC_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : FLASH_IRQHandler
* Description    : This function handles Flash interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : RCC_IRQHandler
* Description    : This function handles RCC interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RCC_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI0_IRQHandler
* Description    : This function handles External interrupt Line 0 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI0_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI1_IRQHandler
* Description    : This function handles External interrupt Line 1 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
extern SemaphoreHandle_t xDisplayMutex;
extern SemaphoreHandle_t xEtapaDoJogo;
extern SemaphoreHandle_t xMusicOnOffMutex;
extern SemaphoreHandle_t xClockMutex;
extern SemaphoreHandle_t xWinLose;
extern SemaphoreHandle_t xDisplayMutex;

extern QueueHandle_t xPlayerDisplayPosQueue;
extern QueueHandle_t xPlayerMapPosQueue;

typedef struct XY_obj_vals
{
	int16_t x;
	int16_t y;
} XY_obj_vals;

typedef struct XYcolor_CDs
{
	int16_t x;
	int16_t y;
	uint16_t cor;
} XYcolor_CDs;

XYcolor_CDs last_cd1;
XYcolor_CDs last_cd2;
XYcolor_CDs last_cd3;

uint8_t history_time;
uint8_t last_y;

uint8_t catch_nr = 0;
uint8_t flag_cor;
uint8_t clock_time;
uint8_t out_end_flag;
uint8_t intro_theme;
//uint8_t
void EXTI1_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line1);
//	GPIO_WriteBit(GPIOB, GPIO_Pin_0, 1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0));


	XY_obj_vals Map_Display_vals;
	if( xPlayerDisplayPosQueue != 0 ) xQueuePeekFromISR( xPlayerDisplayPosQueue, &Map_Display_vals);
	int16_t x_disp = Map_Display_vals.x;
	int16_t y_disp = Map_Display_vals.y;


	uint8_t opt[4] = {10, 90, 70, 20};
	uint8_t x_off = 34;
	uint8_t y_off = 14;
	static BaseType_t pxHigherPriorityTaskWoken;

	uint8_t click_flag = 1;

	///////// Menu ////////
	if (uxSemaphoreGetCount( xEtapaDoJogo ) == 0 && click_flag == 1)
	{
		click_flag = 0;
		// Start
		if( (x_disp > opt[0] && y_disp > opt[1]) && (x_disp < (opt[0]+opt[2]) && y_disp < (opt[1]+opt[3])) )
		{
			xSemaphoreGiveFromISR(xEtapaDoJogo, pxHigherPriorityTaskWoken);
			if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();

			// Follow up mouse position to DVD position //
			XY_obj_vals Pos;
			if (x_disp < 128/2) Pos.x = x_disp - x_off/2;
			else Pos.x = 128*2 - (128-x_disp) - x_off/2;
			Pos.y = 160*2 - (160-y_disp) - y_off/2;
			xQueueReset(xPlayerMapPosQueue);//Clear xPlayerMapPosQueue
			xQueueSendToBackFromISR(xPlayerMapPosQueue, &Pos, &pxHigherPriorityTaskWoken);

			Pos.x = x_disp - x_off/2;
			Pos.y = y_disp - y_off/2;
			xQueueReset(xPlayerDisplayPosQueue);//Clear xPlayerDisplayPosQueue
			xQueueSendToBackFromISR(xPlayerDisplayPosQueue, &Pos, &pxHigherPriorityTaskWoken);
			//////////////////////////////////////////////

			xSemaphoreGiveFromISR(xDisplayMutex, pxHigherPriorityTaskWoken);
			if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();
		}

		//Settings
		if( (x_disp > opt[0] && y_disp > opt[1]+30) && (x_disp < (opt[0]+opt[2]) && y_disp < (opt[1]+30+opt[3])) )
		{
			while (uxSemaphoreGetCount( xEtapaDoJogo ) < 2)
			{
				xSemaphoreGiveFromISR(xEtapaDoJogo, pxHigherPriorityTaskWoken);
				if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();
			}

			xSemaphoreGiveFromISR(xDisplayMutex, pxHigherPriorityTaskWoken);
			if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();
		}
	}

	///////// História ///////// Skip
	if (uxSemaphoreGetCount( xEtapaDoJogo ) == 1 && click_flag == 1 && history_time < 29)
	{
		click_flag = 0;
		history_time = 30;
		last_y = 130;

		while (uxSemaphoreGetCount( xEtapaDoJogo ) < 3)
		{
			xSemaphoreGiveFromISR(xEtapaDoJogo, pxHigherPriorityTaskWoken);
			if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();
		}


		xSemaphoreGiveFromISR(xDisplayMutex, pxHigherPriorityTaskWoken);
		if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();
	}

	//////// Settings boxes ////////
	uint8_t back[4] = {22, 140, 80, 12};
	uint8_t mus[4] = {68, 98, 30, 12};
	if (uxSemaphoreGetCount( xEtapaDoJogo ) == 2 && click_flag == 1)
	{
		click_flag = 0;
		// Back
		if( (x_disp > back[0] && y_disp > back[1]) && (x_disp < (back[0]+back[2]) && y_disp < (back[1]+back[3])) )
		{
			while (uxSemaphoreGetCount( xEtapaDoJogo ) > 0)
			{
				xSemaphoreTakeFromISR(xEtapaDoJogo, pxHigherPriorityTaskWoken);
				if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();
			}

			xSemaphoreGiveFromISR(xDisplayMutex, pxHigherPriorityTaskWoken);
			if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();
		}

		// Toggle Music
		if( (x_disp > mus[0] && y_disp > mus[1]) && (x_disp < (mus[0]+mus[2]) && y_disp < (mus[1]+mus[3])) )
		{
			if (uxSemaphoreGetCount(xMusicOnOffMutex) == 0)
			{
				intro_theme = 0;
				xSemaphoreGiveFromISR( xMusicOnOffMutex, &pxHigherPriorityTaskWoken ); // Turn ON
				if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD(); /* forces the context change */

			}
			else
			{
				xSemaphoreTakeFromISR( xMusicOnOffMutex, &pxHigherPriorityTaskWoken ); // Turn OFF
				if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD(); /* forces the context change */
			}
		}

	}


	/////// End Win/Lose ///////
	if (uxSemaphoreGetCount( xEtapaDoJogo ) == 3 && click_flag == 1)
	{
		if ( last_cd1.cor != 0 && (x_disp > last_cd1.x-x_off && y_disp > last_cd1.y-y_off/2) && (x_disp < (last_cd1.x+x_off) && y_disp < (last_cd1.y+y_off*2)) )
		{
			catch_nr++;
//			GPIO_WriteBit(GPIOB, GPIO_Pin_0, 1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0));
			last_cd1.cor = 0;
		}

		if ( last_cd2.cor != 0 && (x_disp > last_cd2.x-x_off && y_disp > last_cd2.y-y_off) && (x_disp < (last_cd2.x+x_off) && y_disp < (last_cd2.y+y_off*2)) )
		{
			catch_nr++;
//			GPIO_WriteBit(GPIOB, GPIO_Pin_0, 1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0));
			last_cd2.cor = 0;
		}

		if ( last_cd3.cor != 0 && (x_disp > last_cd3.x-x_off && y_disp > last_cd3.y-y_off) && (x_disp < (last_cd3.x+x_off) && y_disp < (last_cd3.y+y_off*2)) )
		{
			catch_nr++;
//			GPIO_WriteBit(GPIOB, GPIO_Pin_0, 1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0));
			last_cd3.cor = 0;
		}

		if (catch_nr > 2)
		{
			xSemaphoreGiveFromISR( xWinLose, &pxHigherPriorityTaskWoken );
			xSemaphoreGiveFromISR(xEtapaDoJogo, &pxHigherPriorityTaskWoken );
			xSemaphoreGiveFromISR(xDisplayMutex, &pxHigherPriorityTaskWoken );
		}

	}


	/////// End Win/Lose ///////
	if (uxSemaphoreGetCount( xEtapaDoJogo ) == 4 && click_flag == 1)
	{
		click_flag = 0;
		if (out_end_flag)
		{
			while (uxSemaphoreGetCount( xEtapaDoJogo ) > 0)
			{
				xSemaphoreTakeFromISR(xEtapaDoJogo, pxHigherPriorityTaskWoken);
				if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();
			}
			out_end_flag = 0;
		}

		xSemaphoreGiveFromISR(xDisplayMutex, pxHigherPriorityTaskWoken);
		if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD();

		catch_nr = 0;
		flag_cor = 1;
		clock_time = 15;
	}

}

/*******************************************************************************
* Function Name  : EXTI2_IRQHandler
* Description    : This function handles External interrupt Line 2 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI2_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI3_IRQHandler
* Description    : This function handles External interrupt Line 3 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI3_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI4_IRQHandler
* Description    : This function handles External interrupt Line 4 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI4_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMA1_Channel1_IRQHandler
* Description    : This function handles DMA1 Channel 1 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel1_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMA1_Channel2_IRQHandler
* Description    : This function handles DMA1 Channel 2 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel2_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMA1_Channel3_IRQHandler
* Description    : This function handles DMA1 Channel 3 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel3_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMA1_Channel4_IRQHandler
* Description    : This function handles DMA1 Channel 4 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel4_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMA1_Channel5_IRQHandler
* Description    : This function handles DMA1 Channel 5 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel5_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMA1_Channel6_IRQHandler
* Description    : This function handles DMA1 Channel 6 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel6_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMA1_Channel7_IRQHandler
* Description    : This function handles DMA1 Channel 7 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel7_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : ADC1_2_IRQHandler
* Description    : This function handles ADC1 and ADC2 global interrupts requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ADC1_2_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : USB_HP_CAN_TX_IRQHandler
* Description    : This function handles USB High Priority or CAN TX interrupts
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_HP_CAN_TX_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : USB_LP_CAN_RX0_IRQHandler
* Description    : This function handles USB Low Priority or CAN RX0 interrupts
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_LP_CAN_RX0_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : CAN_RX1_IRQHandler
* Description    : This function handles CAN RX1 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CAN_RX1_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : CAN_SCE_IRQHandler
* Description    : This function handles CAN SCE interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CAN_SCE_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI9_5_IRQHandler
* Description    : This function handles External lines 9 to 5 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI9_5_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM1_BRK_IRQHandler
* Description    : This function handles TIM1 Break interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM1_BRK_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM1_UP_IRQHandler
* Description    : This function handles TIM1 overflow and update interrupt
*                  request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM1_UP_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM1_TRG_COM_IRQHandler
* Description    : This function handles TIM1 Trigger and commutation interrupts
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM1_TRG_COM_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM1_CC_IRQHandler
* Description    : This function handles TIM1 capture compare interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM1_CC_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM2_IRQHandler
* Description    : This function handles TIM2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
extern SemaphoreHandle_t xClockMutex;
void TIM2_IRQHandler(void)
{
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

	static BaseType_t pxHigherPriorityTaskWoken;
	if (uxSemaphoreGetCountFromISR( xClockMutex ) == 0) //only take Mutex when not already taken
	{
		xSemaphoreGiveFromISR( xClockMutex, &pxHigherPriorityTaskWoken );
		if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD(); /* forces the context change */
	}

}

/*******************************************************************************
* Function Name  : TIM3_IRQHandler
* Description    : This function handles TIM3 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
extern SemaphoreHandle_t xMusicMutex;
void TIM3_IRQHandler(void)
{
	static BaseType_t pxHigherPriorityTaskWoken;
	if (uxSemaphoreGetCountFromISR( xMusicMutex ) == 0) //only take Mutex when not already taken
	{
		xSemaphoreGiveFromISR( xMusicMutex, &pxHigherPriorityTaskWoken );
		if( pxHigherPriorityTaskWoken == pdTRUE ) taskYIELD(); /* forces the context change */
	}


	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

}

/*******************************************************************************
* Function Name  : TIM4_IRQHandler
* Description    : This function handles TIM4 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM4_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : I2C1_EV_IRQHandler
* Description    : This function handles I2C1 Event interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C1_EV_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : I2C1_ER_IRQHandler
* Description    : This function handles I2C1 Error interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C1_ER_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : I2C2_EV_IRQHandler
* Description    : This function handles I2C2 Event interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C2_EV_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : I2C2_ER_IRQHandler
* Description    : This function handles I2C2 Error interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C2_ER_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : SPI1_IRQHandler
* Description    : This function handles SPI1 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPI1_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : SPI2_IRQHandler
* Description    : This function handles SPI2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPI2_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : USART1_IRQHandler
* Description    : This function handles USART1 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USART1_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : USART2_IRQHandler
* Description    : This function handles USART2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint8_t Rx_data;
extern QueueHandle_t xQueue;
void USART2_IRQHandler(void) //ex1
{
//	while( USART_GetFlagStatus( USART2, USART_FLAG_RXNE ) == RESET );
//	Rx_data = USART_ReceiveData(USART2);
//	USART_ClearFlag(USART2, USART_FLAG_RXNE);
//
//	while(USART_GetFlagStatus( USART2, USART_FLAG_TXE) == RESET);
//	USART_SendData(USART2, Rx_data);
//	USART_ClearFlag(USART2, USART_FLAG_TXE);
//
//
//
//	static BaseType_t pxHigherPriorityTaskWoken;
//	uint32_t ulVar;
//
//	if (Rx_data != 10) //Não enviar o '/n' para a Queue
//	{
//		ulVar = Rx_data;
//
//		xQueueSendToBackFromISR( xQueue, &ulVar, &pxHigherPriorityTaskWoken);
//
//		if( pxHigherPriorityTaskWoken == pdTRUE )
//			taskYIELD(); /* Forces a context switch before exit the ISR. */
//	}
//
//
//
//
////	ClearITPendigBit();
}

/*******************************************************************************
* Function Name  : USART3_IRQHandler
* Description    : This function handles USART3 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USART3_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI15_10_IRQHandler
* Description    : This function handles External lines 15 to 10 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI15_10_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : RTCAlarm_IRQHandler
* Description    : This function handles RTC Alarm interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RTCAlarm_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : USBWakeUp_IRQHandler
* Description    : This function handles USB WakeUp interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBWakeUp_IRQHandler(void)
{
}
