/**
  ******************************************************************************
  * @copyright: Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file     : xx.c
  * @author   : ZJY
  * @version  : V1.0
  * @date     : 20xx-xx-xx
  * @brief    : xxx
  *                  1.xx
  *                  2.xx
  *
  * @attention: None
  ******************************************************************************
  * @history  : 
  *      V1.0 : 1.xxx
  *
  *
  *     
  ******************************************************************************
  */
/*------------------------------ include --------------------------------------*/
#include "operate_loop.h"
#include "custom_proto.h"
#include "serial.h"
#include "stimer.h"
#include "crc.h"

#define  LOG_TAG             "operate_loop"
#define  LOG_LVL             1
#include "log.h"
/*------------------------------ Macro definition -----------------------------*/


/*------------------------------ typedef definition ---------------------------*/

/*------------------------------ variables prototypes -------------------------*/
Protocol_type loop_proto = {0};
serial_t *port;

static uint8_t proto_rx_buf[OPERATE_LOOP_FRAME_MAX_LEN] = {0};
static uint8_t proto_tx_buf[OPERATE_LOOP_FRAME_MIN_LEN] = {0};

proto_parser_t custom_parser;

static uint8_t handshake_flag = 0;

/*------------------------------ function prototypes --------------------------*/
static void proto_data_handler(Protocol_type *type, uint8_t cmd, const uint8_t *data, uint16_t len);
static void handshake_ok_handler(Protocol_type *type);

/*------------------------------ application ----------------------------------*/
int operate_loop_init()
{
    int ret;
    
    // 查找串口设备
    port = serial_find("uart3");
    if (port == NULL) {
        LOG_D("Failed to find uart2\r\n");
        return -ENODEV;
    }
    
    // 初始化串口设备
    ret = serial_init(port);
    if (ret != 0) {
        LOG_D("Failed to initialize uart3: %d\r\n", ret);
        return ret;
    }
    struct serial_configure cfg = SERIAL_CONFIG_DEFAULT;
    cfg.baud_rate = BAUD_RATE_115200;
    
    ret = serial_control(port, SERIAL_CMD_SET_CONFIG, &cfg);
    if (ret != 0) {
        LOG_D("Failed to configure uart3: %d\r\n", ret);
        return ret;
    }
    
    loop_proto.m_Addr = 0x03,
    loop_proto.m_Expand = 1;
    loop_proto.m_Addr_Expand = 0x05;
    loop_proto.m_LenMax = OPERATE_LOOP_FRAME_MAX_LEN;
    loop_proto.m_LenMin = OPERATE_LOOP_FRAME_MIN_LEN;
    loop_proto.rx_buff = proto_rx_buf;
    loop_proto.port = port;
    loop_proto.calc_check = (checksum_calculator_t)crc16_modbus;
    loop_proto.check_size = 2;
    
    return 0;
}

/******************************* End Of File ************************************/
