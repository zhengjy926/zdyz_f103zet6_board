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

/* Exported define -----------------------------------------------------------*/
#define CUSTOM_FRAME_MAX_LEN                    256   /**< 最大帧长度 */
#define CUSTOM_PROTO_COMMANDS_NUM_MAX           (32)

/** @defgroup The general command issued by the host
  * @{
  */
#define CUSTOM_PROTO_CMD_HANDSHAKE              (0x01)  /* 上电握手 */
#define CUSTOM_PROTO_CMD_GET_SW_VERSION         (0x02)  /* 查询软件版本信息 */
#define CUSTOM_PROTO_CMD_SET_HW_VERSION	        (0x03)  /* 设置硬件版本号信息 */
#define CUSTOM_PROTO_CMD_GET_HW_VERSION	        (0x04)  /* 查询硬件版本信息 */
#define CUSTOM_PROTO_CMD_SET_SERIAL_NUM         (0x05)  /* 设置序列号信息 */
#define CUSTOM_PROTO_CMD_GET_SERIAL_NUM         (0x06)  /* 查询序列号信息 */
#define CUSTOM_PROTO_CMD_CTRL_SOFT_RESET        (0x07)  /* 系统复位控制 */
#define CUSTOM_PROTO_CMD_SELF_CHECK             (0x08)  /* 系统自检控制 */
#define CUSTOM_PROTO_CMD_LOWPOWER_MODE          (0x09)  /* 低功耗控制 */
#define CUSTOM_PROTO_CMD_CTRL_IAP               (0x0A)  /* 启动在线升级 */
#define CUSTOM_PROTO_CMD_CTRL_UPLOAD            (0x0B)  /* 控制上传模式 */
#define CUSTOM_PROTO_CMD_ASK                    (0x2F)  /* 通用应答命令 */

typedef enum 
{
	ack_Finish, 				//0x00	成功
	ack_Failure_Unknown,		//0x01	失败，未知错误
	ack_Failure_Format,			//0x02	失败，格式异常
	ack_Failure_FrameLen,		//0x03	失败，长度异常
	ack_Failure_EmptyData,		//0x04	失败，数据为空
	ack_Failure_DeviceNumber,	//0x05	失败，设备编号异常
	ack_Failure_Check,			//0x06	失败，校验异常
	
	ack_Failure_Timeout=0x11,	//0x11	失败，执行超时
	ack_Failure_Busy,			//0x12	失败，设备忙
	ack_Failure_DataAbnormal,	//0x13	失败，数据异常
	ack_Failure_OperateAbnormal,//0x14	失败，操作异常
	ack_Failure_ModeAbnormal,	//0x15	失败，模式异常
	ack_Failure_OperateInvalid,	//0x16	失败，操作无效
	ack_Failure_ModeLock,		//0x17	失败，模块锁定
	ack_Failure_SystemLock,		//0x18	失败，系统锁定
}ACK_Item;

/* Exported typedef ----------------------------------------------------------*/
// 命令处理函数类型
typedef void (*ProtoCommandHandler)(const uint8_t *data, uint16_t len);

/* 命令表结构 */
typedef struct {
    uint8_t cmd_code;
    ProtoCommandHandler handler;
} ProtoCommandEntry;

/* Exported function prototypes ----------------------------------------------*/
int custom_proto_init(void);
void custom_proto_task(void);
int custom_proto_send_frame(uint8_t cmd, uint8_t dev_id, uint8_t module_id, const uint8_t *data, uint16_t len);
/* 命令注册接口 */
void proto_register_command(uint8_t cmd_code, ProtoCommandHandler handler);
/* 命令查找接口 */
ProtoCommandHandler proto_find_handler(uint8_t cmd_code);

void custom_proto_handshake_start(void);
void custom_proto_handshake_stop(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DEVICE_PROTO_H__ */
