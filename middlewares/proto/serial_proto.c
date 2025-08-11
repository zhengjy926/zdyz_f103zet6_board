/**
 * @file    serial_proto.c
 * @brief   串口协议解析器实现
 * @author  ZJY
 * @version V1.1
 * @date    2025-01-15
 * @copyright Copyright (c) Hangzhou Dinova EP Technology Co.,Ltd
 */

/* Includes ------------------------------------------------------------------*/
#include "serial_proto.h"
#include "byteorder.h"
#include "minmax.h"
#include <string.h>

#define  DEBUG_TAG                  "serial_proto"
#define  FILE_DEBUG_LEVEL           3
#define  FILE_ASSERT_ENABLED        1
#include "log.h"
#include <assert.h>

/* Private typedef -----------------------------------------------------------*/
static proto_get_tick_func_t s_get_tick_func = NULL;  // 静态函数指针,用于获取系统时间

static void reset_parser(proto_parser_t *parser, uint32_t current_time_ms);
static int validate_and_handle_frame(proto_parser_t *parser);

/* Exported functions --------------------------------------------------------*/
/**
 * @brief 初始化帧解析器
 * @param parser 解析器实例指针
 * @param serial 串口实例指针
 * @param config 帧协议配置指针
 * @param get_tick_func 获取系统时间的函数指针
 */
int frame_parser_init(proto_parser_t       *parser,
                     serial_t             *serial, 
                     const frame_config_t *config,
                     proto_get_tick_func_t get_tick_func)
{
    if (parser == NULL || config == NULL || serial == NULL || get_tick_func == NULL) {
        LOG_E("parser or config or serial or get_tick_func is NULL\r\n");
        return -EINVAL;
    }

    assert(config->type == FRAME_TYPE_FIXED_LEN || 
           config->type == FRAME_TYPE_VAR_LEN_FIELD || 
           config->type == FRAME_TYPE_VAR_LEN_TERMINATOR);
    assert(config->head_bytes != NULL);
    assert(config->head_len > 0 && config->head_len < config->rx_buffsz);
    assert(config->tail_bytes != NULL);
    assert(config->tail_len > 0 && config->tail_len < config->rx_buffsz);
    assert(config->timeout_ms > 0);
    assert(config->max_frame_size > 0 && config->max_frame_size <= config->rx_buffsz);
    assert(config->rx_buffsz > 0 && config->rx_buffsz <= serial->rx_bufsz);
    assert(config->rx_buff != NULL);

    if (config->type == FRAME_TYPE_VAR_LEN_FIELD) {
        assert(config->len_field_offset > 0);
        assert(config->len_field_size > 0);
    } else if (config->type == FRAME_TYPE_FIXED_LEN) {
        assert(config->fixed_len > 0);
    }

    if (config->checksum_size > 0) {
        assert(config->calc_checksum != NULL);
    }
    
    parser->serial = serial;
    parser->config = config;
    s_get_tick_func = get_tick_func;  // 保存获取系统时间的函数指针
    
    // 消息队列创建
    parser->msg_queue = xQueueCreate(32, sizeof(proto_msg_t));
    if (parser->msg_queue == NULL)
        return -ENOMEM;
    
    reset_parser(parser, s_get_tick_func());
    
    return 0;
}

/**
 * @brief 帧处理函数
 * @param parser 解析器实例指针
 */
void frame_parser_process(proto_parser_t *parser)
{
    const frame_config_t *cfg = parser->config;
    size_t linear_size, tail_pos;
    uint32_t current_time_ms = s_get_tick_func();  // 使用函数指针获取当前时间

    // 检查是否超时
    if (parser->state != STATE_FINDING_HEAD &&
        (current_time_ms - parser->last_rx_time > cfg->timeout_ms) ) {
        LOG_D("Protocol timeout!\r\n");
        
        if (kfifo_len(&parser->serial->rx_fifo) >= cfg->head_len) {
            kfifo_skip_count(&parser->serial->rx_fifo, cfg->head_len);
        } else {
            kfifo_skip_count(&parser->serial->rx_fifo, 1);
        }
        reset_parser(parser, current_time_ms);
        return;
    }

    if (kfifo_is_empty(&parser->serial->rx_fifo)) {
        return;
    }

    while(!kfifo_is_empty(&parser->serial->rx_fifo)) 
    {
        switch (parser->state) {
            case STATE_FINDING_HEAD: {
                // 使用零拷贝技术在kfifo的线性区中查找帧头
                linear_size = kfifo_out_linear(&parser->serial->rx_fifo, &tail_pos, kfifo_len(&parser->serial->rx_fifo));
                uint8_t *linear_buf = (uint8_t*)parser->serial->rx_buf + tail_pos;
                
                // memchr查找帧头的第一个字节
                uint8_t *head_pos = (uint8_t *)memchr(linear_buf, cfg->head_bytes[0], linear_size);
                
                if (head_pos) {
                    // 找到了第一个字节
                    size_t junk_len = head_pos - linear_buf;
                    kfifo_skip_count(&parser->serial->rx_fifo, junk_len); // 丢弃前面的无效数据
                    
                    parser->state = STATE_READING_HEAD;
                    parser->last_rx_time = current_time_ms; // 更新时间戳
                    // 不需要break，直接进入下一个状态继续处理
                } else {
                    // 整个线性区都没有找到，全部丢弃
                    kfifo_skip_count(&parser->serial->rx_fifo, linear_size);
                    return; // 等待下一次调用
                }
            }
            
            case STATE_READING_HEAD: {
                if (kfifo_len(&parser->serial->rx_fifo) < cfg->head_len) {
                    // 数据不足以读取完整帧头，等待
                    return;
                }
                
                kfifo_out_peek(&parser->serial->rx_fifo, cfg->rx_buff, cfg->head_len);
                if (memcmp(cfg->rx_buff, cfg->head_bytes, cfg->head_len) == 0) {
                    // 帧头匹配成功
                    switch(cfg->type) {
                        case FRAME_TYPE_FIXED_LEN:
                            parser->current_frame_len = cfg->fixed_len;
                            parser->state = STATE_READING_DATA;
                            break;
                        case FRAME_TYPE_VAR_LEN_FIELD:
                            parser->state = STATE_READING_LEN;
                            break;
                        case FRAME_TYPE_VAR_LEN_TERMINATOR:
                            parser->current_frame_len = 0; // 长度未知
                            parser->state = STATE_READING_DATA;
                            break;
                    }
                    parser->last_rx_time = current_time_ms; // 更新时间戳
                } else {
                    // 帧头不匹配（第一个字节相同，但后续不同），丢弃一个字节后重新寻找
                    kfifo_skip_count(&parser->serial->rx_fifo, 1);
                    LOG_D("Header mismatch!\r\n");
                    reset_parser(parser, current_time_ms);
                }
                break; // 状态切换后，在下一次while循环中处理
            }

            case STATE_READING_LEN: { // Only for FRAME_TYPE_VAR_LEN_FIELD
                size_t bytes_needed = cfg->len_field_offset + cfg->len_field_size;
                if (kfifo_len(&parser->serial->rx_fifo) < bytes_needed) {
                    // 数据不足以读取长度字段
                    return;
                }
                
                kfifo_out_peek(&parser->serial->rx_fifo, cfg->rx_buff, bytes_needed);
                // 根据长度字段大小直接从字节数组中获取长度值
                uint16_t payload_len = 0;
                if (cfg->len_field_size == 1) {
                    payload_len = cfg->rx_buff[cfg->len_field_offset];
                } else if (cfg->len_field_size == 2) {
                    // 默认按小端序读取
                    payload_len = cfg->rx_buff[cfg->len_field_offset] | 
                                 (cfg->rx_buff[cfg->len_field_offset + 1] << 8);
                    
                    // 如果是大端序，则转换
                    if (cfg->is_big_endian) {
                        payload_len = be16_to_cpu(payload_len);
                    }
                }
                
                // 计算总帧长
                if (cfg->calc_frame_len) {
                    // 使用自定义函数计算帧总长度
                    parser->current_frame_len = cfg->calc_frame_len(payload_len, cfg->head_len, cfg->tail_len, cfg->checksum_size);
                } else {
                    // 默认计算方式：帧头 + 数据载荷长度 + 校验 + 帧尾
                    parser->current_frame_len = payload_len + cfg->head_len + cfg->tail_len + cfg->checksum_size;
                }
                
                // **关键的健壮性检查**
                if (parser->current_frame_len < (cfg->head_len + cfg->checksum_size + cfg->tail_len) || 
                    parser->current_frame_len > cfg->max_frame_size) {
                    // 无效的长度（太短或长于最大帧大小限制）
                    // 判定为错误帧，丢弃帧头，重新寻找
                    kfifo_skip_count(&parser->serial->rx_fifo, cfg->head_len);
                    LOG_D("Error frame! Discard header and search again!\r\n");
                    reset_parser(parser, current_time_ms);
                    break;
                }
                parser->state = STATE_READING_DATA;
                break; // 状态切换后，在下一次while循环中处理
            }

            case STATE_READING_DATA: {
                if (cfg->type == FRAME_TYPE_VAR_LEN_TERMINATOR) {
                    // 对于以帧尾结束的类型，我们需要搜索帧尾
                    size_t len = kfifo_len(&parser->serial->rx_fifo);
                    if (len < (cfg->head_len + cfg->tail_len))
                        return; // 数据太少，不可能有完整帧
                    
                    for (size_t i = cfg->head_len; i <= (len - cfg->tail_len); ++i) {
                         kfifo_out_peek(&parser->serial->rx_fifo, cfg->rx_buff, i + cfg->tail_len);
                         if (memcmp(cfg->rx_buff + i, cfg->tail_bytes, cfg->tail_len) == 0) {
                            // 找到了帧尾
                            parser->current_frame_len = i + cfg->tail_len;
                            // 最小帧长度的验证
                            if (parser->current_frame_len < (cfg->head_len + cfg->tail_len + cfg->checksum_size)) {
                                LOG_D("Frame too short: %zu\r\n", parser->current_frame_len);
                                kfifo_skip_count(&parser->serial->rx_fifo, cfg->head_len);
                                reset_parser(parser, current_time_ms);
                                return;
                            }
                            // Fallthrough to validation
                            goto frame_validation;
                         }
                    }
                    // 如果fifo满了还没找到，则丢弃最老的数据
                    if(kfifo_is_full(&parser->serial->rx_fifo)){
                        kfifo_skip_count(&parser->serial->rx_fifo, 1);
                    }
                    return;

                } else { // 对于固定长度和带长度字段的帧
                    if (kfifo_len(&parser->serial->rx_fifo) < parser->current_frame_len) {
                        return; // 数据不足，等待
                    }
                    // Fallthrough to validation
                }

            frame_validation:;
                int result = validate_and_handle_frame(parser);
                if (result == 0) { // 成功
                    // 成功处理了一帧，重置以寻找下一帧
                    reset_parser(parser, current_time_ms);
                } else if (result == -1) { // 帧无效
                    LOG_D("Discarding invalid frame\r\n");
                    kfifo_skip_count(&parser->serial->rx_fifo, 1); // 丢弃已验证的帧头（或一个字节）
                    reset_parser(parser, current_time_ms);
                }
                // 如果 result == 1 (数据不足), 则不做任何事，退出循环等待更多数据
                if(result != 1){
                    continue; // 处理完一帧（无论好坏），立即尝试处理下一帧
                } else {
                    return;
                }
            }
        } // end switch
    } // end while
}

void frame_parser_reset(proto_parser_t *parser)
{
    if (parser == NULL) {
        LOG_E("parser is NULL\r\n");
        return;
    }
    
    reset_parser(parser, s_get_tick_func());
}

int proto_msg_get(proto_parser_t *parser, proto_msg_t *msg)
{
    if (parser == NULL || msg == NULL) {
        LOG_E("parser or msg is NULL\r\n");
        return -EINVAL;
    }

    if (parser->msg_queue == NULL) {
        LOG_E("msg_queue is NULL\r\n");
        return -EINVAL;
    }

    if (xQueueReceive(parser->msg_queue, msg, portMAX_DELAY) != pdPASS) {
        LOG_E("Failed to receive message from queue!\r\n");
        return -1;
    }

    return 0;
}

void proto_msg_free(proto_msg_t *msg)
{  
    vPortFree(msg->data);
    msg->data = NULL;
    msg->len = 0;
}


/* Private functions ---------------------------------------------------------*/
/**
 * @brief 在找到一个潜在的完整帧后，进行校验和处理
 * @return 0: 帧有效并已处理, -1: 帧无效, 1: 数据不足无法判断
 */
static int validate_and_handle_frame(proto_parser_t *parser)
{
    const frame_config_t *cfg = parser->config;
    
    kfifo_out_peek(&parser->serial->rx_fifo, cfg->rx_buff, parser->current_frame_len);

    // 验证帧尾
    if (cfg->type != FRAME_TYPE_VAR_LEN_TERMINATOR) {
        if (memcmp(cfg->rx_buff + parser->current_frame_len - cfg->tail_len, cfg->tail_bytes, cfg->tail_len) != 0) {
            LOG_D("Tail mismatch!\r\n");
            return -1; // 帧尾不匹配
        }
    }

    // 验证校验
    if (cfg->checksum_size > 0 && cfg->calc_checksum != NULL) {
        // 计算校验的数据范围：从帧头开始（不包括帧头），到校验码之前
        size_t data_to_check_len = parser->current_frame_len - cfg->tail_len - cfg->checksum_size - cfg->head_len;
        
        uint32_t calculated_checksum = cfg->calc_checksum(&cfg->rx_buff[cfg->head_len], data_to_check_len);
        
        // 从帧数据中提取接收到的校验码
        uint32_t received_checksum = 0;
        const uint8_t* p_checksum = cfg->rx_buff + data_to_check_len + cfg->head_len;
        
        if(cfg->checksum_size == 1) {
            received_checksum = *p_checksum; // 单字节无需转换字节序
        }
        else if(cfg->checksum_size == 2) {
            uint16_t checksum16 = p_checksum[0] | (p_checksum[1] << 8); // 先按小端序读取
            
            if(cfg->is_big_endian) {
                checksum16 = be16_to_cpu(checksum16); // 如果是大端序，使用转换函数
            }
            received_checksum = checksum16;
        }
        else if(cfg->checksum_size == 4) {
            // 先按小端序读取
            uint32_t checksum32 = p_checksum[0] | (p_checksum[1] << 8) | (p_checksum[2] << 16) | (p_checksum[3] << 24);
            
            if(cfg->is_big_endian) {
                checksum32 = be32_to_cpu(checksum32); // 如果是大端序，使用转换函数
            }
            received_checksum = checksum32;
        }

        if (calculated_checksum != received_checksum) {
            LOG_D("Checksum mismatch!\r\n");
            return -1; // 校验失败
        }
    }
    
    // 计算有效数据位置和长度（去除帧头、帧尾、校验）
    size_t data_offset = cfg->head_len;
    size_t data_len = parser->current_frame_len - cfg->head_len - cfg->tail_len - cfg->checksum_size;
    
    // 推送到消息队列
    if (data_len > 0) {
        proto_msg_t msg;
        
        // 分配并复制有效数据到消息缓冲区
        uint8_t *msg_data = pvPortMalloc(data_len);
        if (msg_data == NULL) {
            LOG_E("Failed to allocate message buffer!\r\n");
            return -1;
        }
        memcpy(msg_data, cfg->rx_buff + data_offset, data_len);
        
        msg.data = msg_data;
        msg.len = data_len;
        
        // 将消息推送到队列中
        if (xQueueSend(parser->msg_queue, &msg, portMAX_DELAY) != pdPASS) {
            LOG_E("Failed to send message to queue!\r\n");
            return -1;
        }
    }

    // 从FIFO中正式丢弃已处理的帧数据
    kfifo_skip_count(&parser->serial->rx_fifo, parser->current_frame_len);
    return 0; // 成功
}

/**
 * @brief 重置解析器状态
 */
static void reset_parser(proto_parser_t *parser, uint32_t current_time_ms)
{
    parser->state = STATE_FINDING_HEAD;
    parser->current_frame_len = 0;
    parser->last_rx_time = current_time_ms;
}
