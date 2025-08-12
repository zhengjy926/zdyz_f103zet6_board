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
#include "serial.h"
#include "crc.h"
#include "byteorder.h"
#include "stimer.h"
#include <string.h>

#define  LOG_TAG             "custom_proto"
#define  LOG_LVL             4
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const uint8_t   custom_header[]                    = {0xFA};
static const uint8_t   custom_tail[]                      = {0x0D};
static       uint8_t   custom_rx_buf[CUSTOM_FRAME_MAX_LEN] = {0};
static       uint8_t   custom_tx_buf[CUSTOM_FRAME_MAX_LEN] = {0};

proto_parser_t custom_parser;
serial_t *port;
static uint8_t handshake_flag = 0;

/* 静态命令表 */
static ProtoCommandEntry command_table[CUSTOM_PROTO_COMMANDS_NUM_MAX];
static size_t command_count = 0;

stimer_t proto_timer;

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
    .checksum_size = 2,
    .calc_checksum = (uint32_t (*)(const uint8_t *, size_t))crc16_modbus,
    .is_big_endian = false,
    .rx_buff    = custom_rx_buf,
    .rx_buffsz  = sizeof(custom_rx_buf),
    .timeout = 100,
    .max_frame_size = CUSTOM_FRAME_MAX_LEN,
    .min_frame_size = 9,
    .on_frame = custom_proto_handle,
};

/* Exported functions --------------------------------------------------------*/
void custom_timer_callback(void* param)
{
    custom_proto_send_frame(CUSTOM_PROTO_CMD_HANDSHAKE, 0x02, 0x05, NULL, 0);
}


/**
 * @brief  初始化自定义协议
 * @return 0成功，负值失败
 */
int custom_proto_init(void)
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
    
    // 初始化帧解析器
    frame_parser_init(&custom_parser, &port->rx_fifo, &custom_frame_config, HAL_GetTick);
    stimer_create(&proto_timer, 1000, STIMER_AUTO_RELOAD, custom_timer_callback, NULL);
    LOG_D("Custom protocol initialized successfully\r\n");
    
    return 0;
}

void proto_register_command(uint8_t cmd_code, 
                           ProtoCommandHandler handler)
{
    if (command_count >= CUSTOM_PROTO_COMMANDS_NUM_MAX) {
        LOG_W("Command table full!\n");
        return;
    }
    
    command_table[command_count++] = (ProtoCommandEntry){
        .cmd_code = cmd_code,
        .handler = handler,
    };
}

ProtoCommandHandler proto_find_handler(uint8_t cmd_code)
{
    for (size_t i = 0; i < command_count; i++) {
        if (command_table[i].cmd_code == cmd_code) {
            return command_table[i].handler;
        }
    }
    return NULL;
}

static void custom_proto_handle(const uint8_t *payload, size_t len, void *user_data)
{
    // 设备地址与模块地址校验
    if (payload[2] != 0x02 || payload[4] != 0x05) {
        LOG_D("Device id or module id is errno!\r\n");
        return;
    }
    
    uint8_t cmd = payload[3];
    
    if (handshake_flag) {
        // 查找命令处理器
        ProtoCommandHandler handler = proto_find_handler(cmd);
        
        if (!handler) {
            LOG_W("Unknown command: 0x%02X\r\n", cmd);
            return;
        }
        
        len = len - 5;
        // 执行命令处理
        handler(&payload[5], len);
    } else {
        if (cmd == CUSTOM_PROTO_CMD_HANDSHAKE) {
            handshake_flag = 1;
            stimer_stop(&proto_timer);
            LOG_D("Handshake successful!\r\n");
            custom_proto_send_frame(CUSTOM_PROTO_CMD_HANDSHAKE, 0x02, 0x05, NULL, 0);
        } else {
            LOG_W("Please shake hands first!\r\n");
        }
    }
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
int custom_proto_send_frame(uint8_t cmd, uint8_t dev_id, uint8_t module_id, const uint8_t *data, uint16_t len)
{
    uint16_t frame_len;
    uint16_t checksum;
    uint16_t i = 0;
    int ret;
    
    // 计算帧长度: 帧头(1) + 长度(2) + 设备ID(1) + 命令(1) + 模块ID(1) + 数据(len) + 校验和(2) + 帧尾(1)
    frame_len = 9 + len;
    
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
