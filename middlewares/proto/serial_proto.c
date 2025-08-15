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

#define  LOG_TAG             "serial_proto"
#define  LOG_LVL             1
#include "log.h"
#include <assert.h>

/* Private typedef -----------------------------------------------------------*/
static void reset_parser(proto_parser_t *parser, uint32_t now_time);
static int validate_and_handle_frame(proto_parser_t *parser);

/* Exported functions --------------------------------------------------------*/
/**
 * @brief 初始化帧解析器
 * @param p 解析器实例指针
 * @param fifo 串口实例指针
 * @param cfg 帧协议配置指针
 * @param get_tick_func 获取系统时间的函数指针
 */
int frame_parser_init(proto_parser_t       *p,
                      kfifo_t              *fifo,
                      const frame_cfg_t *cfg,
                      proto_get_tick_func_t get_tick_func)
{
    if (!p || !fifo || !cfg || !cfg->head_bytes || cfg->head_len == 0 || !cfg->tail_bytes || cfg->tail_len == 0 || !cfg->rx_buff || cfg->rx_buffsz == 0 || !get_tick_func)
        return -1;
    if (kfifo_esize(fifo) != 1) {
        LOG_E("kfifo esize must be 1 (byte FIFO)\r\n");
        return -2;
    }
    if (cfg->max_frame_size == 0 || cfg->max_frame_size > cfg->rx_buffsz) {
        LOG_E("max_frame_size must be >0 and <= rx_buffsz\r\n");
        return -3;
    }
    if (cfg->type == FRAME_TYPE_VAR_LEN_FIELD && cfg->len_field_size == 0)
        return -4;
    if (cfg->type == FRAME_TYPE_FIXED_LEN && cfg->fixed_len == 0)
        return -5;
    if (cfg->checksum_size && !cfg->calc_checksum)
        return -6;
    
    // (核心修改) 校验用户配置的 min_frame_size
    if (cfg->min_frame_size == 0 || cfg->min_frame_size > cfg->max_frame_size) {
        LOG_E("min_frame_size must be >0 and <= max_frame_size\r\n");
        return -7;
    }
    // 安全性检查：最小帧长至少要能容纳 帧头+帧尾+校验
    if (cfg->min_frame_size < (cfg->head_len + cfg->tail_len + cfg->checksum_size)) {
        LOG_E("min_frame_size is smaller than protocol overhead (head+tail+checksum)\r\n");
        return -8;
    }
    if (cfg->type == FRAME_TYPE_FIXED_LEN && cfg->min_frame_size != cfg->fixed_len) {
        LOG_W("For fixed length frames, min_frame_size should be equal to fixed_len\r\n");
    }

    p->cfg = cfg;
    p->fifo = fifo;
    p->get_tick = get_tick_func;

    frame_parser_reset(p, p->get_tick());
    
    return 0;
}

/**
 * @brief 帧处理函数
 * @param p 解析器实例指针
 */
void frame_parser_process(proto_parser_t *p)
{
    const frame_cfg_t *cfg = p->cfg;
    uint32_t now_time = p->get_tick();

    if (p->state != STATE_FINDING_HEAD && (now_time - p->last_time > cfg->timeout)) {
        LOG_D("Protocol timeout!\r\n");   
        kfifo_skip(p->fifo);
        reset_parser(p, now_time);
        return;
    }
    
    // (核心修改) 直接使用用户配置的最小帧长作为准入条件
    while(kfifo_len(p->fifo) >= cfg->min_frame_size) 
    {
        switch (p->state)
        {
            case STATE_FINDING_HEAD: {
                size_t tail_pos;
                size_t linear_size = kfifo_out_linear(p->fifo, &tail_pos, kfifo_size(p->fifo));
                uint8_t *linear_buf = (uint8_t*)p->fifo->data + tail_pos;
                
                uint8_t *head_pos = (uint8_t *)memchr(linear_buf, cfg->head_bytes[0], linear_size);
                
                if (head_pos) {
                    size_t junk_len = head_pos - linear_buf;
                    if(junk_len > 0) kfifo_skip_count(p->fifo, junk_len);

                    if (kfifo_len(p->fifo) < cfg->head_len) {
                        return;
                    }

                    kfifo_out_peek(p->fifo, cfg->rx_buff, cfg->head_len);
                    if (memcmp(cfg->rx_buff, cfg->head_bytes, cfg->head_len) == 0) {
                        LOG_D("The header matching was successful!\r\n");
                        p->last_time = now_time;
                        switch(cfg->type) {
                            case FRAME_TYPE_FIXED_LEN:
                                p->current_frame_len = cfg->fixed_len;
                                p->state = STATE_READING_DATA;
                                break;
                            case FRAME_TYPE_VAR_LEN_FIELD:
                                p->state = STATE_READING_LEN;
                                break;
                            case FRAME_TYPE_VAR_LEN_TERMINATOR:
                                p->current_frame_len = 0;
                                p->state = STATE_READING_DATA;
                                break;
                        }
                    } else {
                        LOG_D("Header mismatch after finding first byte!\r\n");
                        kfifo_skip(p->fifo);
                    }
                } else {
                    kfifo_skip_count(p->fifo, linear_size);
                    return;
                }
                break;
            }

            case STATE_READING_LEN: {
                size_t bytes_needed = cfg->len_field_offset + cfg->len_field_size;
                if (kfifo_len(p->fifo) < bytes_needed) {
                    return;
                }
                
                kfifo_out_peek(p->fifo, cfg->rx_buff, bytes_needed);
                
                uint16_t payload_len = 0;
                const uint8_t* p_len = cfg->rx_buff + cfg->len_field_offset;

                if (cfg->len_field_size == 1) {
                    payload_len = *p_len;
                } else if (cfg->len_field_size == 2) {
                    payload_len = *(uint16_t*)p_len;
                    if (cfg->is_big_endian) {
                        payload_len = be16_to_cpu(payload_len);
                    }
                }
                
                if (cfg->calc_frame_len) {
                    p->current_frame_len = cfg->calc_frame_len((struct frame_parser*)p, payload_len);
                } else {
                    p->current_frame_len = payload_len + cfg->head_len + cfg->tail_len + cfg->checksum_size;
                }
                
                if (p->current_frame_len < cfg->min_frame_size || p->current_frame_len > cfg->max_frame_size) {
                    LOG_D("Invalid frame length! Discard header and search again!\r\n");
                    kfifo_skip(p->fifo);
                    reset_parser(p, now_time);
                } else {
                    p->state = STATE_READING_DATA;
                }
                break; 
            }

            case STATE_READING_DATA: {
                if (cfg->type == FRAME_TYPE_VAR_LEN_TERMINATOR) {
                    size_t len = kfifo_len(p->fifo);
                    int found_tail = 0;
                    for (size_t i = cfg->head_len; i <= (len - cfg->tail_len); ++i) {
                         kfifo_out_peek(p->fifo, cfg->rx_buff, i + cfg->tail_len);
                         if (memcmp(cfg->rx_buff + i, cfg->tail_bytes, cfg->tail_len) == 0) {
                            p->current_frame_len = i + cfg->tail_len;
                            if (p->current_frame_len > cfg->max_frame_size) {
                                LOG_D("Frame too long: %zu, discard and restart!\r\n", p->current_frame_len);
                                kfifo_skip(p->fifo); //
                                reset_parser(p, now_time);
                                found_tail = -1; // 特殊标记，用于跳出外层循环
                            } else {
                                found_tail = 1;
                            }
                            break; // 退出 for 循环
                         }
                    }

                    if (found_tail == 1) {
                        goto frame_validation;
                    } else if (found_tail == 0) {
                        if(kfifo_is_full(p->fifo)){
                            LOG_D("Terminator not found and fifo is full, discard oldest byte\r\n");
                            kfifo_skip(p->fifo); //
                        }
                        return; // 未找到帧尾，等待更多数据
                    }
                    // 如果 found_tail == -1 (帧超长)，则直接跳到循环末尾
                    goto frame_validation;
                } else {
                    if (kfifo_len(p->fifo) < p->current_frame_len) {
                        return;
                    }
                }

            frame_validation:;
                int result = validate_and_handle_frame(p);
                if (result < 0) {
                    LOG_D("Discarding invalid frame\r\n");
                    kfifo_skip(p->fifo);
                }
                reset_parser(p, now_time);
                break;
            }
        }
    }
}

void frame_parser_reset(proto_parser_t *p, uint32_t now)
{
    if (!p) return;
    reset_parser(p, now);
}

/* Private functions ---------------------------------------------------------*/
static int validate_and_handle_frame(proto_parser_t *parser)
{
    const frame_cfg_t *cfg = parser->cfg;
    
    kfifo_out_peek(parser->fifo, cfg->rx_buff, parser->current_frame_len);

    if (cfg->type != FRAME_TYPE_VAR_LEN_TERMINATOR) {
        if (memcmp(cfg->rx_buff + parser->current_frame_len - cfg->tail_len, cfg->tail_bytes, cfg->tail_len) != 0) {
            LOG_D("Tail mismatch!\r\n");
            return -1;
        }
    }

    if (cfg->checksum_size > 0 && cfg->calc_checksum != NULL) {
        size_t data_to_check_len = parser->current_frame_len - cfg->head_len - cfg->tail_len - cfg->checksum_size;
        
        uint32_t calculated_checksum = cfg->calc_checksum(&cfg->rx_buff[cfg->head_len], data_to_check_len);
        const uint8_t* p_checksum = cfg->rx_buff + cfg->head_len + data_to_check_len;
        uint32_t received_checksum = 0;
        
        if(cfg->checksum_size == 1) {
            received_checksum = *p_checksum;
        } else if(cfg->checksum_size == 2) {
            received_checksum = *(uint16_t*)p_checksum;
            if(cfg->is_big_endian) received_checksum = be16_to_cpu(received_checksum);
        } else if(cfg->checksum_size == 4) {
            received_checksum = *(uint32_t*)p_checksum;
            if(cfg->is_big_endian) received_checksum = be32_to_cpu(received_checksum);
        }

        if (calculated_checksum != received_checksum) {
            LOG_D("Checksum mismatch! calculated: %08X, received: %08X\r\n", calculated_checksum, received_checksum);
            return -2;
        }
    }
    
    size_t data_offset = cfg->head_len;
    size_t data_len = parser->current_frame_len - cfg->head_len - cfg->tail_len - cfg->checksum_size;
    
    if (cfg->on_frame && data_len > 0) {
        cfg->on_frame(cfg->rx_buff + data_offset, data_len, cfg->user_data);
    }

    kfifo_skip_count(parser->fifo, parser->current_frame_len);
    return 0;
}

static void reset_parser(proto_parser_t *parser, uint32_t now_time)
{
    parser->state = STATE_FINDING_HEAD;
    parser->current_frame_len = 0;
    parser->last_time = now_time;
}