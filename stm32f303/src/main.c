/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f3xx_hal.h"
#include "adc.h"
#include "opamp.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include <math.h>
#include "defines.h"
#include "hal.h"
#include "angle.h"
#include "usbd_cdc_if.h"
#include "version.h"
#include "common.h"
#include "commands.h"

uint32_t systick_freq;

// //hal interface TODO: move hal interface to file
// void hal_enable_rt(){
//    // TIM_Cmd(TIM4, ENABLE);
// }
// void hal_enable_frt(){
//    // TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
// }
// void hal_disable_rt(){
//    // TIM_Cmd(TIM4, DISABLE);
// }
// void hal_disable_frt(){
//    // TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
// }

uint32_t hal_get_systick_value(){
   return(SysTick->VAL);
}

uint32_t hal_get_systick_reload(){
   return(SysTick->LOAD);
}

uint32_t hal_get_systick_freq(){
   return(systick_freq);
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */


void TIM8_UP_IRQHandler(){
   __HAL_TIM_CLEAR_IT(&htim8, TIM_IT_UPDATE);
   hal_run_rt();
}

void bootloader(char * ptr){
   RTC->BKP0R = 0xDEADBEEF;
   NVIC_SystemReset();
}

COMMAND("bootloader", bootloader);

int main(void)
{

  /* USER CODE BEGIN 1 */
  SCB->VTOR = 0x8004000;
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();
  systick_freq = HAL_RCC_GetHCLKFreq();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM8_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();

  // MX_DAC_Init();
  MX_OPAMP2_Init();
  // MX_USART1_UART_Init();
  // MX_USART3_UART_Init();
  MX_USB_DEVICE_Init();
  __HAL_RCC_DMA1_CLK_ENABLE();
  // __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_RTC_ENABLE();

  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
  HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);

  HAL_OPAMP_SelfCalibrate(&hopamp2);
  
  //USB EN IO board: PB15
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);

  if (HAL_OPAMP_Start(&hopamp2) != HAL_OK){
    Error_Handler();
  }

  htim8.Instance->CCR1 = 0;
  htim8.Instance->CCR2 = 0;
  htim8.Instance->CCR3 = 0;
  
  HAL_ADC_Start(&hadc1);
  HAL_ADC_Start(&hadc2);

  if (HAL_TIM_Base_Start_IT(&htim8) != HAL_OK){
 	Error_Handler();
  }

  hal_init();
  // hal load comps
  load_comp(comp_by_name("term"));
  load_comp(comp_by_name("sim"));
  load_comp(comp_by_name("io"));
  
  hal_parse("term0.rt_prio = 0.1");
  hal_parse("ls0.rt_prio = 0.6");
  hal_parse("io0.rt_prio = 1.0");
  hal_parse("dq0.rt_prio = 2.0");
  hal_parse("curpid0.rt_prio = 3.0");
  hal_parse("idq0.rt_prio = 4.0");
  hal_parse("svm0.rt_prio = 5.0");
  hal_parse("hv0.rt_prio = 6.0");
  
  hal_parse("term0.send_step = 50.0");
  hal_parse("term0.gain0 = 10.0");
  hal_parse("term0.gain1 = 10.0");
  hal_parse("term0.gain2 = 10.0");
  hal_parse("term0.gain3 = 10.0");
  hal_parse("term0.gain4 = 1.0");
  hal_parse("term0.gain5 = 10.0");
  hal_parse("term0.gain6 = 10.0");
  hal_parse("term0.gain7 = 10.0");
  hal_parse("curpid0.max_cur = 25.0");
  /*
  //link LS
  hal_parse("io0.mot_temp", "ls0.mot_temp");
  hal_parse("io0.udc", "ls0.dc_volt");
  hal_parse("io0.hv_temp", "ls0.hv_temp");
  hal_parse("ls0.d_cmd", "curpid0.id_cmd");
  hal_parse("ls0.q_cmd", "curpid0.iq_cmd");
  hal_parse("ls0.pos", "idq0.pos");
  hal_parse("ls0.pos", "dq0.pos");
  hal_parse("ls0.en", "hv0.en");
  
  //ADC TEST
  hal_parse("io0.udc", "term0.wave3");
  hal_parse("io0.udc", "hv0.udc");
  hal_parse("io0.iu", "dq0.u");
  hal_parse("io0.iv", "dq0.v");
  hal_parse("io0.iw", "dq0.w");
  
  // hal_parse("sim0.vel", "idq0.pos");
  // hal_parse("sim0.vel", "dq0.pos");
  
  hal_parse("idq0.u", "svm0.u");
  hal_parse("idq0.v", "svm0.v");
  hal_parse("idq0.w", "svm0.w");
  hal_parse("svm0.su", "hv0.u");
  hal_parse("svm0.sv", "hv0.v");
  hal_parse("svm0.sw", "hv0.w");
  hal_parse("io0.udc", "svm0.udc");
  hal_parse("io0.hv_temp", "hv0.hv_temp");
  
  hal_parse("dq0.d", "curpid0.id_fb");
  hal_parse("dq0.q", "curpid0.iq_fb");
  
  hal_parse("dq0.d", "ls0.d_fb");
  hal_parse("dq0.q", "ls0.q_fb");
  
  hal_parse("io0.u", "ls0.u_fb");
  hal_parse("io0.v", "ls0.v_fb");
  hal_parse("io0.w", "ls0.w_fb");
  
  hal_parse("curpid0.ud", "idq0.d");
  hal_parse("curpid0.uq", "idq0.q");
  
  hal_parse("curpid0.id_cmd", "term0.wave0");
  hal_parse("curpid0.iq_cmd", "term0.wave1");

  hal_parse("curpid0.id_fb", "term0.wave2");
  hal_parse("curpid0.iq_fb", "term0.wave3");
  
  hal_parse("term0.con", "io0.led");

  hal_parse("ls0.r", "curpid0.rd");
  hal_parse("ls0.r", "curpid0.rq");
  hal_parse("ls0.l", "curpid0.ld");
  hal_parse("ls0.l", "curpid0.lq");
  hal_parse("ls0.psi", "curpid0.psi");
  hal_parse("ls0.cur_p", "curpid0.kp");
  hal_parse("ls0.cur_i", "curpid0.ki");
  hal_parse("ls0.cur_ff", "curpid0.ff");
  hal_parse("ls0.cur_ind", "curpid0.kind");
  hal_parse("ls0.max_cur", "curpid0.max_cur");
  hal_parse("ls0.pwm_volt", "curpid0.pwm_volt");
  hal_parse("ls0.vel", "curpid0.vel");
  */
  // hal parse config
  hal_init_nrt();
  // error foo
  hal_start();
  
  while (1)
  {
     hal_run_nrt();
     cdc_poll();
     HAL_Delay(1);
  }

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_TIM8
                              |RCC_PERIPHCLK_ADC12|RCC_PERIPHCLK_ADC34|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_SYSCLK;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
  PeriphClkInit.Adc34ClockSelection = RCC_ADC34PLLCLK_DIV1;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  PeriphClkInit.Tim8ClockSelection = RCC_TIM8CLK_PLLCLK;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

//Delay implementation for hal_term.c
void Wait(uint32_t ms){
   HAL_Delay(ms);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
