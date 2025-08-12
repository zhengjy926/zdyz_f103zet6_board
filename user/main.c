/**
  ******************************************************************************
  * @file        : main.c
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 2024-09-26
  * @brief       : Main program body
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "board.h"
#include "gpio.h"

#include "stimer.h"
#include "key.h"

#include "data_mgmt.h"

#include <stdio.h>
#include <assert.h>

#define  LOG_TAG             "main"
#define  LOG_LVL             4
#include "log.h"

#include "stm32_flash.h"
#include "mtd_core.h"
#include <string.h>

#include "app.h"
#include "custom_proto.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
size_t pin_id = 0;

uint8_t r_buf[16];
uint8_t w_buf[16] = "hello nihao!";
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void)
{   
    board_init();
    
    pin_id = gpio_get("PB.0");
    
    /* set led gpio mode */
    gpio_set_mode(LED_RED_PIN_ID, PIN_OUTPUT_PP, PIN_PULL_UP);
    gpio_set_mode(LED_BLUE_PIN_ID, PIN_OUTPUT_PP, PIN_PULL_UP);
    gpio_write(LED_RED_PIN_ID, 0);
    gpio_write(LED_BLUE_PIN_ID, 0);
    
    LOG_D("Init success!\r\n");
    LOG_W("LOG_LVL_WARNING success!\r\n");
    LOG_E("LOG_LVL_ERROR success!\r\n");
    
    data_mgmt_init();
    custom_proto_init();
    custom_proto_handshake_start();
    
    while (1)
    {
        key_event_msg_t msg;
        
        stimer_service();
        custom_proto_task();
        
        if (key_get_event(&msg))
        {
            LOG_D("key_id = %d, key_event = %d\r\n", msg.id, msg.event);
        }
    }
}
