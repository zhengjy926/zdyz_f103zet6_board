/**
  ******************************************************************************
  * @copyright   : Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file        : device_proto.h
  * @author      : ZJY
  * @version     : V1.1
  * @date        : 2025-01-15
  * @brief       : 设备通信协议头文件 - 统一协议模块
  * 
  * @details     本文件定义了设备通信的统一协议接口，包括：
  *              - 自定义设备通信协议
  *              - 统一的错误处理和调试功能
  * 
  * @attention   使用前需要先调用相应的初始化函数
  * 
  * @copyright   Copyright (c) 2024 ZJY
  ******************************************************************************
  * @history     
  *         V1.0 : 初始版本，实现设备组自定义串口通信协议
  ******************************************************************************
  */
#ifndef __CUSTOM_PROTO_H__
#define __CUSTOM_PROTO_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"
#include "serial_proto.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported define -----------------------------------------------------------*/
#define CUSTOM_FRAME_MAX_LEN     256  /**< 自定义协议最大帧长度 */
#define CUSTOM_MSG_BUFFER_SIZE   16   /**< 自定义协议消息缓冲区大小 */
#define CUSTOM_MAX_FRAME_SIZE    512  /**< 自定义协议最大帧大小限制 */

#define CUSTOM_PROTO_DEVICE_ADDR 0x01  /**< 自定义协议设备地址 */
#define CUSTOM_PROTO_MODULE_ADDR 0x02  /**< 自定义协议模块地址 */

/* Exported typedef ----------------------------------------------------------*/
/**
 * @brief 协议类型枚举
 */
typedef enum {
    PROTO_TYPE_LCD,      /**< LCD串口屏协议 */
    PROTO_TYPE_CUSTOM,   /**< 自定义设备协议 */
} proto_type_t;

/* Exported function prototypes ----------------------------------------------*/
/* 初始化函数 */
int slcd_proto_init(void);
int custom_proto_init(void);

/* 协议打包和发送函数 */
int slcd_proto_send_frame(uint16_t cmd, const uint8_t *data, uint16_t len);
int custom_proto_send_frame(uint8_t cmd, const uint8_t *data, uint16_t len);

extern proto_parser_t custom_parser;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DEVICE_PROTO_H__ */
