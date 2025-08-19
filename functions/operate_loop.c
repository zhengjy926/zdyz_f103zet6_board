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
#define  LOG_LVL             4
#include "log.h"

#include <string.h>
/*------------------------------ Macro definition -----------------------------*/
static loop_msg_t loop_msg_buf[16];
kfifo_t loop_msg_fifo;

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
static void operate_loop_data_handle(const uint8_t *payload, size_t len, void *user_data);

/*------------------------------ application ----------------------------------*/
int operate_loop_init(void)
{
    int ret;
    
    // 查找串口设备
    port = serial_find("uart3");
    if (port == NULL) {
        LOG_D("Failed to find uart3\r\n");
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
    loop_proto.rx_buffsz = OPERATE_LOOP_FRAME_MAX_LEN;
    loop_proto.m_TxBuffer = proto_tx_buf;
    loop_proto.port = port;
    loop_proto.calc_check = (checksum_calculator_t)crc16_modbus;
    loop_proto.check_size = 2;
    
    loop_proto.frame_cfg.on_frame = operate_loop_data_handle;
    loop_proto.frame_cfg.user_data = (void*)&loop_proto;
    
    // 初始化自定义协议
    ret = custom_proto_init(&loop_proto);
    if (ret != 0) {
        LOG_E("Failed to initialize custom protocol: %d\r\n", ret);
        return -1;
    }
    
    // 循环缓冲区初始化
    kfifo_init(&loop_msg_fifo, loop_msg_buf, sizeof(loop_msg_buf), sizeof(loop_msg_t));
    
    return 0;
}

/**
  * @brief : 发送多字节数据的应答
  * @param :
  * @retval:
  */
void operate_loop_send_string(uint8_t id, uint8_t *pData, uint16_t length)
{
	custom_proto_send_frame(&loop_proto, id, 0, 0, pData, length);
}
/**
  * @brief : 发送单字节数据的应答
  * @param :
  * @retval:
  */
void operate_loop_send_byte(uint8_t id,uint8_t data)
{
	custom_proto_send_frame(&loop_proto, id, 0, 0, &data, 1);
}
/**
  * @brief : 
  * @param :
  * @retval:
  */
void operate_loop_send_cmd(uint8_t id)
{
	custom_proto_send_frame(&loop_proto, id, 0, 0, 0, 0);
}

static void operate_loop_data_handle(const uint8_t *payload, size_t len, void *user_data)
{
    Protocol_type *type = (Protocol_type*)user_data;
    loop_msg_t msg = {0};
    
    if (!type)
        return;
    
    uint8_t dev_id = payload[2];
    msg.id = payload[3];
    
    // 检查帧是否是发给我们的
    if (dev_id != type->m_Addr) {
        LOG_D("Frame not for us. For: 0x%02X, Us: 0x%02X\r\n", dev_id, type->m_Addr);
        operate_loop_send_byte(upLoopImpd_UniversalACK, ack_Failure_DeviceNumber);
        return;
    }

	/*验证扩展地址--系统级通讯无模块地址*/
	if(type->m_Expand & 0x01)
	{
        uint8_t expand = payload[4];
		if(expand != type->m_Addr_Expand)
		{
			//应答一个模块地址不对
			operate_loop_send_byte(upLoopImpd_UniversalACK, ack_Failure_DeviceNumber);
			return;
		}
        msg.len = len - 5;
	} else {
        msg.len = len - 4;
    }
    
    if (msg.len > 0)
        memcpy(msg.buf, &payload[5], msg.len);
    
    kfifo_in(&loop_msg_fifo, &msg, 1);
}

size_t operate_loop_get_msg_len(void)
{
    return kfifo_len(&loop_msg_fifo);
}

size_t operate_loop_get_msg(loop_msg_t *msg)
{
    return kfifo_out(&loop_msg_fifo, msg, 1);
}

/******************************* End Of File ************************************/
