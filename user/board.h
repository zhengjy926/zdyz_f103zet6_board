/**
  ******************************************************************************
  * @copyright   : Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file        : xxx.h
  * @author      : ZJY
  * @version     : V1.0
  * @data        : 20xx-xx-xx
  * @brief       : 
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  *
  *
  ******************************************************************************
  */
#ifndef __BOARD_H__
#define __BOARD_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"

#if defined(SOC_SERIES_STM32F1)
    #include "stm32f1xx.h"
#elif defined(SOC_SERIES_STM32F4)
    #include "stm32f4xx.h"
#elif defined(SOC_SERIES_STM32G4)
    #include "stm32g4xx.h"
#else
#error "Please select first the soc series used in your application!"    
#endif
/* Exported define -----------------------------------------------------------*/
/* 外设宏定义 */
/* 内部 flash 宏定义 */
#define STM32_FLASH_START_ADDR          (FLASH_BASE + (64 - 4) * 1024UL)
#define STM32_FLASH_END_ADDR            (FLASH_BASE + (64UL * 1024) - 1)
#define STM32_FLASH_ERASE_SIZE          (128 * 1024)

#define LOOP_IMPD_ADC_Pin               GPIO_PIN_4
#define LOOP_IMPD_ADC_GPIO_Port         GPIOA

#define IMPD_RELAY_Pin                  GPIO_PIN_15
#define IMPD_RELAY_GPIO_Port            GPIOA

/* GPIO 按键引脚定义 */
#define KEY0_PIN_ID                     (68)
#define KEY1_PIN_ID                     (67)
#define KEY2_PIN_ID                     (66)
#define WKUP_PIN_ID                     (0)

enum key_id {
    KEY0,
    KEY1,
    KEY2,
    WKUP_KEY,
    KEY_ID_MAX
};

/* GPIO LED 引脚定义 */
#define LED_RED_PIN_ID                  (21)
#define LED_BLUE_PIN_ID                 (69)

/* Exported typedef ----------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void board_init(void);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BOARD_H__ */

