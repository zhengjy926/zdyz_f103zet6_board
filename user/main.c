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
#include "frame_parser.h"
#include "crc.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
size_t pin_id = 0;

uint8_t r_buf[16];
uint8_t w_buf[16] = "hello nihao!";
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
void on_frame_received(void *user_data, const uint8_t *payload, size_t len)
{
    printf("----> FRAME RECEIVED (User Data: %s) | len: %zu | payload: ", (char*)user_data, len);
    for(size_t i = 0; i < len; ++i) printf("%02X ", payload[i]);
    printf("<----\n\n");
}

void on_parse_error(void *user_data, error_code_t error)
{
    const char* err_str = "UNKNOWN";
    switch(error) {
        case ERR_TIMEOUT: err_str = "TIMEOUT"; break;
        case ERR_INVALID_LENGTH: err_str = "INVALID LENGTH"; break;
        case ERR_BAD_TAIL: err_str = "BAD TAIL"; break;
        case ERR_BAD_CHECKSUM: err_str = "BAD CHECKSUM"; break;
        default: break;
    }
    LOG_D("!!!! PARSE ERROR (User Data: %s): %s !!!!\n\n", (char*)user_data, err_str);
}

static const uint8_t   custom_header[] = {0xFA};
static const uint8_t   custom_tail[] = {0x0D};
static const protocol_def_t custom_frame_config = {
    .frame_type = FRAME_TYPE_LEN_PREFIX,
    .header = custom_header,
    .header_len = sizeof(custom_header),
    .tail = custom_tail,
    .tail_len = sizeof(custom_tail),
    .len_prefix_params = { .offset = 1, .size = 1, .len_includes_all = false },
    .checksum_params = { .calc = (uint32_t (*)(const uint8_t *, size_t))crc16_modbus, .size = 1 },
    .endianness = ENDIAN_LITTLE,
    .max_frame_len = 64,
    .inter_byte_timeout_ms = 10,
    .frame_timeout_ms = 100,
};
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
    
    while (1)
    {
        key_event_msg_t msg;
        
        stimer_service();
        
        if (key_get_event(&msg))
        {
            LOG_D("key_id = %d, key_event = %d\r\n", msg.id, msg.event);
        }
    }
}
