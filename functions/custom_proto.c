/**
  ******************************************************************************
  * @file        device_proto.c
  * @author      ZJY
  * @version     V1.1
  * @date        2025-01-15
  * @brief       设备组通信协议实现文件
  * 
  * @details     本文件实现了设备通信的统一协议功能，包括：
  *              - 自定义设备通信协议
  *              - 统一的错误处理和调试功能
  * 
  * @attention   使用前需要先调用相应的初始化函数
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "custom_proto.h"
#include "serial_proto.h"
#include "crc.h"
#include "byteorder.h"
#include "app.h"
#include "stimer.h"
#include <string.h>

#define  LOG_TAG             "custom_proto"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const uint8_t custom_header[] = {0xFA};
static const uint8_t custom_tail[]   = {0x0D};

/* Exported variables  -------------------------------------------------------*/
extern uint32_t HAL_GetTick(void);

/* Private function prototypes -----------------------------------------------*/
static void custom_proto_handle(const uint8_t *payload, size_t len, void *user_data);
static size_t custom_calc_frame_len(struct frame_parser *p, uint16_t payload_len)
{
    return payload_len;
}

/* Exported functions --------------------------------------------------------*/
/**
 * @brief  初始化自定义协议
 * @return 0成功，负值失败
 */
int custom_proto_init(Protocol_type *type)
{
    if (!type || !type->port || !type->rx_buff || !type->m_TxBuffer) {
        LOG_E("Invalid parameters in custom_proto_init\r\n");
        return -EINVAL;
    }

    if (type->m_LenMin > type->m_LenMax) {
        LOG_E("Invalid frame length configuration: min > max\r\n");
        return -EINVAL;
    }

    frame_cfg_t *cfg = &type->frame_cfg;
    
    // 基础帧格式配置
    cfg->type = FRAME_TYPE_VAR_LEN_FIELD;
    cfg->head_bytes = custom_header;
    cfg->head_len = sizeof(custom_header);
    cfg->tail_bytes = custom_tail;
    cfg->tail_len = sizeof(custom_tail);
    
    // 长度字段配置
    cfg->len_field_offset = 1;
    cfg->len_field_size = 2;
    cfg->calc_frame_len = custom_calc_frame_len;
    
    // 超时和缓冲区配置
    cfg->timeout = 100;
    cfg->rx_buff = type->rx_buff;
    cfg->rx_buffsz = type->m_LenMax;
    
    // 校验和帧长度配置
    cfg->checksum_size = type->check_size;
    cfg->calc_checksum = type->calc_check;
    cfg->is_big_endian = false;
    cfg->max_frame_size = type->m_LenMax;
    cfg->min_frame_size = type->m_LenMin;
    
//    // 回调函数配置
//    cfg->on_frame = custom_proto_handle;
//    cfg->user_data = type;
    
    // 初始化帧解析器
    int ret = frame_parser_init(&type->parser, &type->port->rx_fifo, cfg, HAL_GetTick);
    if (ret != 0) {
        LOG_E("Frame parser init failed with code %d\r\n", ret);
        return ret;
    }

    LOG_D("Custom protocol initialized successfully!\r\n");
    return 0;
}

//static void custom_proto_handle(const uint8_t *payload, size_t len, void *user_data)
//{
//    Protocol_type *type = (Protocol_type*)user_data;
//    if (!type)
//        return;
//    
//    uint8_t dev_id = payload[FrameAddr];
//    uint8_t cmd = payload[FrameCMD];
//    
//    // 检查帧是否是发给我们的
//    if (dev_id != type->m_Addr) {
//        LOG_D("Frame not for us. For: 0x%02X, Us: 0x%02X\r\n", dev_id, type->m_Addr);
//        return;
//    }

//    LOG_D("Frame received for us. Cmd: 0x%02X\r\n", cmd);

//    // 将数据传递给应用层进行处理
//    if (type->on_data_received) {
//        const uint8_t *app_data = payload + 5; // 实际数据在payload头之后
//        uint16_t app_data_len = len - 5;
//        type->on_data_received(type, cmd, app_data, app_data_len);
//    }
//}

/**
 * @brief 需要周期性调用的协议任务。
 * @param type 指向协议实例的指针。
 */
void custom_proto_parser(Protocol_type *type)
{
    if (type) {
        frame_parser_process(&type->parser);
    }
}
    
/**
 * @brief 发送自定义协议帧
 * @param cmd 命令码
 * @param data 数据内容指针，可以为NULL
 * @param dev_id 设备 ID
 * @param module_id 模块 ID
 * @param len 数据长度
 * @return 0成功，负值失败
 */
int custom_proto_send_frame(Protocol_type *type, uint8_t id, uint8_t id_Expand, uint16_t SN_Expand, const uint8_t *data, uint16_t len)
{
    if (!type)
        return -EINVAL;

    // 计算帧长
    uint16_t frame_len = type->m_LenMin + len;
    
    if (frame_len > type->m_LenMax) {
        LOG_E("Frame too large (%d > %d)\r\n", frame_len, type->m_LenMax);
        return -EINVAL;
    }
    
    if (len > 0 && data == NULL) {
        LOG_E("Invalid data pointer for non-zero length\r\n");
        return -EINVAL;
    }
    
    uint16_t i = 0;
    // 使用协议实例的发送缓冲区
    uint8_t* tx_buf = type->m_TxBuffer;

    tx_buf[i++] = custom_header[0]; // 帧头
    
    // 长度字段 (总帧长)
    tx_buf[i++] = frame_len & 0xFF;
    tx_buf[i++] = (frame_len >> 8) & 0xFF;
    
    tx_buf[i++] = type->m_Addr;
    tx_buf[i++] = id;          // 命令字段
    
	if(type->m_Expand & 0x01)
	{
		tx_buf[i++] = type->m_Addr_Expand;
	}
    
	if(type->m_Expand & 0x02)
	{
		tx_buf[i++] = id_Expand;
	}
	
	if(type->m_Expand & 0x04)
	{
        tx_buf[i++] = SN_Expand & 0xFF;
        tx_buf[i++] = (SN_Expand >> 8) & 0xFF;
	}
    
    // 数据字段
    memcpy(&tx_buf[i], data, len);
    i += len;
    
    // 校验和从长度字段开始计算，直到数据结束
    uint16_t checksum = crc16_modbus(tx_buf + 1, i - 1); 
    
    // 添加校验和 (低字节在前)
    tx_buf[i++] = checksum & 0xFF;
    tx_buf[i++] = (checksum >> 8) & 0xFF;
    
    tx_buf[i++] = custom_tail[0]; // 帧尾
    
    // 发送帧
    int ret = serial_write(type->port, tx_buf, i); // 'i' 现在是总长度
    if (ret != i) {
        LOG_E("Failed to send frame: expected %d, sent %d\r\n", i, ret);
        return -EIO;
    }
    
    LOG_D("Sent frame: cmd=0x%02X, len=%d\r\n", id, len);
    return 0;
}
/* Private functions ---------------------------------------------------------*/ 
