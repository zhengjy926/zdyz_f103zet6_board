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
#define  LOG_LVL             1
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

static const frame_cfg_t custom_frame_config = {
    .type = FRAME_TYPE_VAR_LEN_FIELD,
    .head_bytes = custom_header,
    .head_len = sizeof(custom_header),
    .tail_bytes = custom_tail,
    .tail_len = sizeof(custom_tail),
    .len_field_offset = sizeof(custom_header),
    .len_field_size = 2,
    .calc_frame_len = custom_calc_frame_len,
    .is_big_endian = false,

    .max_frame_size = CUSTOM_FRAME_MAX_LEN,
    .min_frame_size = 9,
    .on_frame = custom_proto_handle,
};

/* Exported functions --------------------------------------------------------*/
void custom_timer_callback(void* param)
{
    custom_proto_send_frame(cmd_HandShake, 0x02, 0x05, NULL, 0);
}


/**
 * @brief  初始化自定义协议
 * @return 0成功，负值失败
 */
int custom_proto_init(proto_parser_t *p, frame_cfg_t *cfg, Protocol_type *type)
{
    cfg->type = FRAME_TYPE_VAR_LEN_FIELD;
    cfg->head_bytes = custom_header;
    cfg->head_len = sizeof(custom_header);
    cfg->tail_bytes = custom_tail;
    cfg->tail_len = sizeof(custom_tail);
    cfg->len_field_offset = sizeof(custom_header);
    cfg->len_field_size = 2;
    cfg->calc_frame_len = custom_calc_frame_len;
    cfg->is_big_endian = false;
    cfg->timeout = 100;
    
    cfg->rx_buff = type->rx_buff;
    cfg->rx_buffsz = type->m_LenMax;
    cfg->checksum_size = type->check_size;
    cfg->calc_checksum = type->calc_check;
    cfg->max_frame_size = type->m_LenMax;
    cfg->min_frame_size = type->m_LenMin;
    
    // 初始化帧解析器
    frame_parser_init(p, &type->port->rx_fifo, cfg, HAL_GetTick);
    stimer_create(&type->timer, 1000, STIMER_AUTO_RELOAD, custom_timer_callback, NULL);
    LOG_D("Custom protocol initialized successfully!\r\n");
    
    return 0;
}

static void custom_proto_handle(const uint8_t *payload, size_t len, void *user_data)
{

}

void custom_proto_task(void)
{
    frame_parser_process(&custom_parser);
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
int custom_proto_send_frame(Protocol_type *type, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    uint16_t frame_len;
    uint16_t checksum;
    uint16_t i = 0;
    int ret;
    
    // 计算帧长度
    frame_len = type->m_LenMin + len;
    
    // 检查缓冲区大小是否足够
    if (frame_len > type->m_LenMax) {
        LOG_D("Frame too large (%d > %d)\r\n", frame_len, type->m_LenMax);
        return -EINVAL;
    }
    
    // 检查数据指针有效性
    if (data == NULL) {
        LOG_D("Invalid data pointer\r\n");
        return -EINVAL;
    }
    
    custom_tx_buf[i++] = custom_header[0]; // 帧头
    
    // 长度字段
    custom_tx_buf[i++] = frame_len & 0xFF;
    custom_tx_buf[i++] = (frame_len >> 8) & 0xFF;
    
    custom_tx_buf[i++] = dev_id;    // 设备id
    custom_tx_buf[i++] = cmd;       // 命令字段
    custom_tx_buf[i++] = module_id; // 模块id
    
    // 数据字段
    if (data != NULL && len > 0) {
        memcpy(&custom_tx_buf[i], data, len);
        i += len;
    }
    
    checksum = crc16_modbus(custom_tx_buf + 1, i-1); // 计算校验和（使用CRC16 Modbus）
    
    // 添加校验和（低字节在前）
    custom_tx_buf[i++] = checksum & 0xFF;
    custom_tx_buf[i++] = (checksum >> 8) & 0xFF;
    
    custom_tx_buf[i++] = custom_tail[0]; // 帧尾
    
    // 发送帧
    ret = serial_write(port, custom_tx_buf, frame_len);
    if (ret != frame_len) {
        LOG_D("Failed to send frame: %d\r\n", ret);
        return -EIO;
    }
    
    LOG_D("Sent frame: cmd=0x%02X, len=%d\r\n", cmd, len);
    
    return 0;
}

void custom_proto_handshake_start(void)
{
    stimer_start(&proto_timer);
}

void custom_proto_handshake_stop(void)
{
    stimer_stop(&proto_timer);
}
/* Private functions ---------------------------------------------------------*/ 
