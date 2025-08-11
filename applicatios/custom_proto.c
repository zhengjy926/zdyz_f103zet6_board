/**
  ******************************************************************************
  * @file        device_proto.c
  * @author      ZJY
  * @version     V1.1
  * @date        2025-01-15
  * @brief       设备通信协议实现文件 - 统一协议模块
  * 
  * @details     本文件实现了设备通信的统一协议功能，包括：
  *              - LCD串口屏通信协议
  *              - 自定义设备通信协议
  *              - 统一的错误处理和调试功能
  * 
  * @attention   使用前需要先调用相应的初始化函数
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "custom_proto.h"
#include "serial_proto.h"
#include "serial.h"
#include "crc.h"
#include "byteorder.h"
#include <string.h>

#define  DEBUG_TAG                  "device_proto"
#define  FILE_DEBUG_LEVEL           3
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* 自定义协议相关定义 */
static const uint8_t   custom_header[]                    = {0xFA};
static const uint8_t   custom_tail[]                      = {0x0D};
static       uint8_t   custom_rx_buf[CUSTOM_FRAME_MAX_LEN] = {0};
static       uint8_t   custom_tx_buf[CUSTOM_FRAME_MAX_LEN] = {0};

/* Private variables ---------------------------------------------------------*/
frame_parser_t         custom_parser;

/* Exported variables  -------------------------------------------------------*/
extern uint32_t HAL_GetTick(void);

/* Private function prototypes -----------------------------------------------*/
static size_t custom_calc_frame_len(uint16_t payload_len, size_t head_len, size_t tail_len, size_t checksum_size)
{
    return head_len + payload_len + checksum_size + tail_len;
}

static const frame_config_t custom_frame_config = {
    .type = FRAME_TYPE_VAR_LEN_FIELD,
    .head_bytes = custom_header,
    .head_len = sizeof(custom_header),
    .tail_bytes = custom_tail,
    .tail_len = sizeof(custom_tail),
    .len_field_offset = sizeof(custom_header),
    .len_field_size = 2,
    .calc_frame_len = custom_calc_frame_len,
    .checksum_size = 2,
    .calc_checksum = (uint32_t (*)(const uint8_t *, size_t))crc16_modbus,
    .is_big_endian = false,
    .rx_buff    = custom_rx_buf,
    .rx_buffsz  = sizeof(custom_rx_buf),
    .timeout_ms = 100,
    .max_frame_size = CUSTOM_MAX_FRAME_SIZE,
};

/* Exported functions --------------------------------------------------------*/
/**
 * @brief 初始化自定义协议
 * @return 0成功，负值失败
 */
int custom_proto_init(void)
{
    serial_t *serial;
    int ret;
    
    // 查找串口设备
    serial = serial_find("uart3");
    if (serial == NULL) {
        LOG_D("Failed to find uart2\r\n");
        return -ENODEV;
    }
    
    // 初始化串口设备
    ret = serial_init(serial);
    if (ret != 0) {
        LOG_D("Failed to initialize uart3: %d\r\n", ret);
        return ret;
    }
    
    struct serial_configure cfg = SERIAL_CONFIG_DEFAULT;
    cfg.baud_rate = BAUD_RATE_115200;

    ret = serial_control(serial, SERIAL_CMD_SET_CONFIG, &cfg);
    if (ret != 0) {
        LOG_D("Failed to configure uart3: %d\r\n", ret);
        return ret;
    }
    
    // 初始化帧解析器
    frame_parser_init(&custom_parser, serial, &custom_frame_config, xTaskGetTickCount);
    LOG_D("Custom protocol initialized successfully\r\n");
    
    return 0;
}

/**
 * @brief 发送自定义协议帧
 * @param cmd 命令码
 * @param data 数据内容指针，可以为NULL
 * @param len 数据长度
 * @return 0成功，负值失败
 */
int custom_proto_send_frame(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    uint16_t frame_len;
    uint16_t checksum;
    uint16_t i = 0;
    int ret;
    
    if (custom_parser.serial == NULL) {
        LOG_D("Protocol not initialized\r\n");
        return -ENODEV;
    }
    
    // 计算帧长度: 帧头(1) + 长度(2) + 设备地址(1) + 命令(1) + 模块地址(1) + 数据(len) + 校验和(2) + 帧尾(1)
    frame_len = 1 + 2 + 1 + 1 + 1 + len + 2 + 1;
    
    // 检查缓冲区大小是否足够
    if (frame_len > CUSTOM_FRAME_MAX_LEN) {
        LOG_D("Frame too large (%d > %d)\r\n", frame_len, CUSTOM_FRAME_MAX_LEN);
        return -EINVAL;
    }
    
    // 检查数据指针有效性
    if (len > 0 && data == NULL) {
        LOG_D("Invalid data pointer\r\n");
        return -EINVAL;
    }
    
    // 帧头
    custom_tx_buf[i++] = custom_header[0];
    
    // 长度字段
    custom_tx_buf[i++] = frame_len & 0xFF;
    custom_tx_buf[i++] = (frame_len >> 8) & 0xFF;
    
    // 设备地址
    custom_tx_buf[i++] = CUSTOM_PROTO_DEVICE_ADDR;
    
    // 命令字段
    custom_tx_buf[i++] = cmd;
    
    // 模块地址
    custom_tx_buf[i++] = CUSTOM_PROTO_MODULE_ADDR;
    
    // 数据字段
    if (data != NULL && len > 0) {
        memcpy(&custom_tx_buf[i], data, len);
        i += len;
    }
    
    // 计算校验和（使用CRC16 Modbus）
    checksum = crc16_modbus(custom_tx_buf + 1, i-1);
    
    // 添加校验和（低字节在前）
    custom_tx_buf[i++] = checksum & 0xFF;
    custom_tx_buf[i++] = (checksum >> 8) & 0xFF;
    
    // 帧尾
    custom_tx_buf[i++] = custom_tail[0];
    
    // 发送帧
    ret = serial_write(custom_parser.serial, custom_tx_buf, frame_len);
    if (ret != frame_len) {
        LOG_D("Failed to send frame: %d\r\n", ret);
        return -EIO;
    }
    
    LOG_D("Sent frame: cmd=0x%02X, len=%d\r\n", cmd, len);
    
    return 0;
}

/* Private functions ---------------------------------------------------------*/ 
