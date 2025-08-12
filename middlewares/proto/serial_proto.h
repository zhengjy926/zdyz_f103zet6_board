/**
  ******************************************************************************
  * @copyright   : Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file        : serial_proto.h
  * @author      : ZJY
  * @version     : V1.1
  * @data        : 2025-01-15
  * @brief       : 串口协议解析器头文件 - 混合架构设计
  * @attention   : 支持紧急事件回调和常规事件消息队列的混合架构
  ******************************************************************************
  * @history     :
  *         V1.0 : 基础协议解析功能
  ******************************************************************************
  */
#ifndef __SERIAL_PROTO_H__
#define __SERIAL_PROTO_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "sys_def.h"
#include "kfifo.h"

/* Exported define -----------------------------------------------------------*/
struct frame_parser_s;
struct frame_parser;

/**
 * @brief 帧格式类型枚举
 */
typedef enum {
    FRAME_TYPE_FIXED_LEN,         /**< 固定长度帧 */
    FRAME_TYPE_VAR_LEN_FIELD,     /**< 带长度字段的变长帧 */
    FRAME_TYPE_VAR_LEN_TERMINATOR /**< 以帧尾结束的变长帧 */
} frame_type_t;

/**
 * @brief 解析状态枚举
 */
typedef enum {
    STATE_FINDING_HEAD,
    STATE_READING_LEN,
    STATE_READING_DATA,
} parser_state_t;

/**
 * @brief 校验计算函数指针原型
 */
typedef uint32_t (*checksum_calculator_t)(const uint8_t *data, size_t len);

/**
 * @brief 计算帧总长度的函数指针类型
 */
typedef size_t (*calc_frame_len_t)(struct frame_parser *p, uint16_t payload_len);

/**
 * @brief Function pointer type for getting system time
 */
typedef uint32_t (*proto_get_tick_func_t)(void);

/**
 * @brief 当一个有效数据帧被解析时调用的回调函数原型
 */
typedef void (*on_frame_received_t)(const uint8_t *payload, size_t len, void *user_data);

/**
 * @brief 消息结构
 */
typedef struct {
    uint8_t* data;              /**< 指向有效数据（不包含帧头、帧尾、校验） */
    uint16_t len;               /**< 有效数据大小 */
} proto_msg_t;

/**
 * @brief 帧协议配置结构体
 */
typedef struct {
    frame_type_t type;                  /**< 帧类型 */
    const uint8_t *head_bytes;          /**< 帧头字节序列 */
    size_t head_len;                    /**< 帧头长度 */
    const uint8_t *tail_bytes;          /**< 帧尾字节序列 */
    size_t tail_len;                    /**< 帧尾长度 */
    
    size_t fixed_len;                   /**< 固定帧总长度（仅用于 FRAME_TYPE_FIXED_LEN） */
    
    // 仅用于 FRAME_TYPE_VAR_LEN_FIELD
    size_t len_field_offset;            /**< 长度字段在帧内的偏移，第一个长度字节的下标索引 */
    size_t len_field_size;              /**< 长度字段本身的字节数 */
    calc_frame_len_t calc_frame_len;    /**< 计算帧总长度的函数 */

    size_t checksum_size;                /**< 校验字段字节数 (0表示无校验) */
    checksum_calculator_t calc_checksum; /**< 校验计算函数 */
    bool is_big_endian;                  /**< 大端/小端序判断 */

    uint32_t timeout;                   /**< 接收超时时间 (ms) */
    size_t max_frame_size;              /**< 最大帧大小限制 */
    size_t min_frame_size;              /**< 预计算的最小帧长 */
    on_frame_received_t on_frame;       /**< 成功解析一帧数据后的回调函数 */
    void *user_data;                    /**< 传递给回调函数的用户自定义数据 */
    
    uint8_t *rx_buff;                   /**< 指向帧接收缓冲区，用于解析数据 */
    uint16_t rx_buffsz;                 /**< 接收缓冲区大小 */
} frame_cfg_t;

/**
 * @brief 帧解析器主结构体
 */
typedef struct frame_parser {
    const frame_cfg_t *cfg;             /**< 指向帧协议配置 */
    parser_state_t state;               /**< 当前状态机状态 */
    kfifo_t *fifo;
    proto_get_tick_func_t get_tick;     /**< 获取系统时间的函数指针 */
    uint32_t last_time;                 /**< 上次接收到数据的时间戳 */
    size_t found_head_len;              /**< 已找到的帧头长度 */
    size_t current_frame_len;           /**< 当前正在解析的帧的总长度 */
} proto_parser_t;

/* Exported function prototypes ----------------------------------------------*/
// 初始化和管理函数
int  frame_parser_init(proto_parser_t *p, kfifo_t *fifo,
                       const frame_cfg_t *cfg,
                       proto_get_tick_func_t get_tick_func);
void frame_parser_reset(proto_parser_t *p, uint32_t now);
void frame_parser_process(proto_parser_t *p);

int proto_msg_get(proto_parser_t *parser, proto_msg_t *msg);
void proto_msg_free(proto_msg_t *msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SERIAL_PROTO_H__ */

