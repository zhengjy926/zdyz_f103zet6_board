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
#include "serial.h"
#include "stimer.h"

/* Exported define -----------------------------------------------------------*/
#define CUSTOM_FRAME_MAX_LEN                    256   /**< 最大帧长度 */

#define PROTO_HEADER                            0xFA
#define PROTO_TAIL                              0x0D

#define FrameAddr                   3 //ID在数据帧第3字节
#define FrameCMD                    4 //CMD在数据第4字节

/** @defgroup The general command issued by the host
  * @{
  */
#define cmd_HandShake               (0x01)  /* 上电握手 */
#define cmd_Get_SoftwareVersion     (0x02)  /* 查询软件版本信息 */
#define cmd_Set_HardwareVersion     (0x03)  /* 设置硬件版本号信息 */
#define cmd_Get_HardwareVersion     (0x04)  /* 查询硬件版本信息 */
#define cmd_Set_SerialNumber        (0x05)  /* 设置序列号信息 */
#define cmd_Get_SerialNumber        (0x06)  /* 查询序列号信息 */
#define cmd_Ctrl_SoftReset          (0x07)  /* 系统复位控制 */
#define cmd_Ctrl_SelfCheck          (0x08)  /* 系统自检控制 */
#define cmd_Ctrl_LowPowerMode       (0x09)  /* 低功耗控制 */
#define cmd_Ctrl_IAP                (0x0A)  /* 启动在线升级 */
#define cmd_Ctrl_UploadMode         (0x0B)  /* 控制上传模式 */
#define cmd_up_UniversalACK         (0x2F)  /* 错误应答命令 */

struct Protocol_type;

typedef void (*data_handler_t)(struct Protocol_type *type, uint8_t cmd, const uint8_t *data, uint16_t len);

typedef enum
{
	ack_Finish = 0, 		    //0x00	成功
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

typedef struct
{
    uint16_t m_Addr;        /**< 设备地址 */
    uint8_t m_Expand;       /**< 扩展标记：位0 扩展地址；位1 扩展功能码；位2 扩展SN码 */
    uint8_t m_Addr_Expand;  /**< 扩展地址 */
    uint8_t m_SN_Expand;    /**< 扩展序列号 */
    uint16_t m_LenMax;      /**< 实际使用的协议帧最大帧长 */
    uint16_t m_LenMin;      /**< 实际使用的协议帧最小帧长（如有扩展位应包含） */
    uint8_t *rx_buff;       /**< 指向帧接收缓冲区，用于解析数据 */
    uint16_t rx_buffsz;     /**< 接收缓冲区大小 */
	uint8_t* m_TxBuffer;    /**< 发送帧缓存区 */
    serial_t *port;         /**< 串口设备指针 */
    uint32_t (*calc_check)(const uint8_t *data, size_t len);    /**< 校验计算函数 */
    uint8_t check_size;     /**< 校验字节数 */
    stimer_t timer;         /**< 软件定时器 */
    proto_parser_t parser;  /**< 协议解析器 */
    frame_cfg_t frame_cfg;  /**< 帧配置 */
    /* 回调函数 */
    data_handler_t on_data_received;/**< 数据接收回调 */
}Protocol_type;

/* Exported typedef ----------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
int custom_proto_init(Protocol_type *type);
void custom_proto_parser(Protocol_type *type);
int custom_proto_send_frame(Protocol_type *type, uint8_t id, uint8_t id_Expand, uint16_t SN_Expand, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DEVICE_PROTO_H__ */
