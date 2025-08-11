/**
 * @file        frame_parser.c
 * @author      Gemini
 * @version     1.1
 * @date        2024-07-17
 * @brief       Implementation of the generic serial frame parser.
 * @details     This file contains the core state machine logic for parsing
 * serial data streams based on a configurable protocol definition.
 * It now uses a tick-based timeout mechanism.
 */

#include "frame_parser.h"
#include <string.h> // For memcmp, memchr

// --- Private Helper Functions ---

/**
 * @brief Resets the parser state to its initial condition (searching for a new frame).
 * @param ctx The parser context.
 * @param consume_count Number of bytes to discard from the buffer before resetting.
 */
static void reset_parser(parser_context_t *ctx, size_t consume_count) {
    ctx->state = STATE_SYNC_SEARCH;
    ctx->bytes_to_consume = consume_count;
}

/**
 * @brief Extracts a multi-byte value from the data stream.
 * @param ptr Pointer to the start of the field in the buffer.
 * @param size The size of the field (1, 2, or 4).
 * @param endian The endianness of the stream.
 * @return The extracted value.
 */
static uint32_t get_val_from_stream(const uint8_t *ptr, size_t size, endianness_t endian) {
    uint32_t value = 0;
    if (endian == ENDIAN_LITTLE) {
        for (size_t i = 0; i < size; ++i) {
            value |= ((uint32_t)ptr[i] << (i * 8));
        }
    } else { // BIG_ENDIAN
        for (size_t i = 0; i < size; ++i) {
            value = (value << 8) | ptr[i];
        }
    }
    return value;
}


/**
 * @brief The core FSM step function. It is executed inside the main processing loop.
 * @param ctx The parser context.
 * @param data_ptr Pointer to the contiguous data block from the buffer.
 * @param data_len Length of the data block.
 */
static void fsm_step(parser_context_t *ctx, const uint8_t *data_ptr, size_t data_len) {
    const protocol_def_t *proto = ctx->protocol;

    switch (ctx->state) {
        case STATE_SYNC_SEARCH: {
            const uint8_t *found = memchr(&data_ptr[ctx->scan_offset], proto->header[0], data_len - ctx->scan_offset);

            if (found) {
                size_t junk_len = found - &data_ptr[ctx->scan_offset];
                ctx->bytes_to_consume = junk_len;
                ctx->scan_offset += junk_len;

                ctx->internal_state.frame.frame_start_offset = ctx->scan_offset;
                ctx->internal_state.sync.matched_bytes = 1;

                if (proto->header_len > 1) {
                    ctx->state = STATE_RECEIVING_HEADER;
                } else {
                    if (proto->frame_type == FRAME_TYPE_LEN_PREFIX) {
                        ctx->state = STATE_EXTRACT_LENGTH;
                    } else {
                        ctx->state = STATE_RECEIVING_PAYLOAD;
                    }
                }
                // Start timing now that we have a potential frame
                if (ctx->get_tick) ctx->start_tick = ctx->get_tick();
            } else {
                ctx->scan_offset = data_len;
            }
            break;
        }

        case STATE_RECEIVING_HEADER: {
            size_t bytes_needed = proto->header_len - ctx->internal_state.sync.matched_bytes;
            size_t bytes_available = data_len - ctx->scan_offset;
            size_t bytes_to_check = (bytes_needed < bytes_available) ? bytes_needed : bytes_available;

            if (memcmp(&data_ptr[ctx->scan_offset], &proto->header[ctx->internal_state.sync.matched_bytes], bytes_to_check) != 0) {
                reset_parser(ctx, ctx->internal_state.frame.frame_start_offset + 1);
                return;
            }

            ctx->scan_offset += bytes_to_check;
            ctx->internal_state.sync.matched_bytes += bytes_to_check;

            if (ctx->internal_state.sync.matched_bytes == proto->header_len) {
                if (proto->frame_type == FRAME_TYPE_LEN_PREFIX) {
                    ctx->state = STATE_EXTRACT_LENGTH;
                } else {
                    ctx->state = STATE_RECEIVING_PAYLOAD;
                }
            }
            // Reset inter-byte timeout
            if (ctx->get_tick) ctx->start_tick = ctx->get_tick();
            break;
        }

        case STATE_EXTRACT_LENGTH: {
            const size_t len_field_size = proto->len_prefix_params.size;
            const size_t len_field_offset = proto->len_prefix_params.offset;
            
            if ((data_len - ctx->internal_state.frame.frame_start_offset) >= (len_field_offset + len_field_size)) {
                const uint8_t* len_ptr = &data_ptr[ctx->internal_state.frame.frame_start_offset + len_field_offset];
                size_t parsed_len = get_val_from_stream(len_ptr, len_field_size, proto->endianness);

                size_t total_len;
                if (proto->len_prefix_params.len_includes_all) {
                     total_len = parsed_len;
                } else {
                     total_len = proto->header_len + parsed_len + proto->checksum_params.size + proto->tail_len;
                }

                if (total_len > proto->max_frame_len || total_len == 0) {
                    if (ctx->callbacks.on_parse_error) ctx->callbacks.on_parse_error(ctx->user_data, ERR_INVALID_LENGTH);
                    reset_parser(ctx, ctx->internal_state.frame.frame_start_offset + 1);
                    return;
                }
                
                ctx->internal_state.frame.expected_len = total_len;
                ctx->state = STATE_RECEIVING_PAYLOAD;
                // Reset timeout for full frame reception
                if (ctx->get_tick) ctx->start_tick = ctx->get_tick();
            }
            break;
        }

        case STATE_RECEIVING_PAYLOAD: {
            size_t total_received_len = data_len - ctx->internal_state.frame.frame_start_offset;
            
            if (proto->frame_type == FRAME_TYPE_FIXED_LEN) {
                ctx->internal_state.frame.expected_len = proto->fixed_len_params.frame_total_len;
            }
            
            if (proto->frame_type == FRAME_TYPE_FIXED_LEN || proto->frame_type == FRAME_TYPE_LEN_PREFIX) {
                if (total_received_len >= ctx->internal_state.frame.expected_len) {
                    ctx->state = STATE_VALIDATE_FRAME;
                }
            } else { // FRAME_TYPE_DELIMITER
                if (proto->tail && proto->tail_len > 0) {
                    for (size_t i = ctx->scan_offset; i <= data_len - proto->tail_len; ++i) {
                         if (memcmp(&data_ptr[i], proto->tail, proto->tail_len) == 0) {
                             ctx->internal_state.frame.expected_len = (i + proto->tail_len) - ctx->internal_state.frame.frame_start_offset;
                             ctx->state = STATE_VALIDATE_FRAME;
                             return;
                         }
                    }
                }
            }

            if (total_received_len > proto->max_frame_len) {
                if (ctx->callbacks.on_parse_error) ctx->callbacks.on_parse_error(ctx->user_data, ERR_INVALID_LENGTH);
                reset_parser(ctx, ctx->internal_state.frame.frame_start_offset + 1);
                return;
            }
            ctx->scan_offset = data_len;
            break;
        }

        case STATE_VALIDATE_FRAME: {
            const uint8_t* frame_start_ptr = &data_ptr[ctx->internal_state.frame.frame_start_offset];
            size_t frame_len = ctx->internal_state.frame.expected_len;

            if (proto->tail_len > 0 && proto->frame_type != FRAME_TYPE_DELIMITER) {
                const uint8_t* expected_tail_ptr = frame_start_ptr + frame_len - proto->tail_len;
                if (memcmp(expected_tail_ptr, proto->tail, proto->tail_len) != 0) {
                    if (ctx->callbacks.on_parse_error) ctx->callbacks.on_parse_error(ctx->user_data, ERR_BAD_TAIL);
                    reset_parser(ctx, ctx->internal_state.frame.frame_start_offset + 1);
                    return;
                }
            }
            
            if (proto->checksum_params.calc) {
                size_t chk_size = proto->checksum_params.size;
                size_t data_part_len = frame_len - proto->header_len - chk_size - proto->tail_len;
                const uint8_t* data_to_checksum_ptr = frame_start_ptr + proto->header_len;
                
                uint32_t calculated_checksum = proto->checksum_params.calc(data_to_checksum_ptr, data_part_len);
                const uint8_t* received_checksum_ptr = data_to_checksum_ptr + data_part_len;
                uint32_t received_checksum = get_val_from_stream(received_checksum_ptr, chk_size, proto->endianness);

                if (calculated_checksum != received_checksum) {
                    if (ctx->callbacks.on_parse_error) ctx->callbacks.on_parse_error(ctx->user_data, ERR_BAD_CHECKSUM);
                    reset_parser(ctx, ctx->internal_state.frame.frame_start_offset + 1);
                    return;
                }
            }

            if (ctx->callbacks.on_frame_received) {
                const uint8_t* payload_ptr = frame_start_ptr + proto->header_len;
                size_t payload_len = frame_len - proto->header_len - proto->checksum_params.size - proto->tail_len;
                ctx->callbacks.on_frame_received(ctx->user_data, payload_ptr, payload_len);
            }
            
            reset_parser(ctx, ctx->internal_state.frame.frame_start_offset + frame_len);
            break;
        }
    }
}


// --- Public API Implementation ---

void parser_init(
    parser_context_t *ctx,
    const protocol_def_t *protocol,
    buffer_if_t *buffer_if,
    get_tick_func_t get_tick,
    callbacks_t callbacks,
    void *user_data)
{
    memset(ctx, 0, sizeof(parser_context_t));
    ctx->protocol = protocol;
    ctx->buffer_if = buffer_if;
    ctx->get_tick = get_tick;
    ctx->callbacks = callbacks;
    ctx->user_data = user_data;
    ctx->state = STATE_SYNC_SEARCH;
}

void parser_process(parser_context_t *ctx) {
    // 1. Check for timeout if we are in the middle of receiving a frame.
    if (ctx->state != STATE_SYNC_SEARCH && ctx->get_tick) {
        // Note: This handles tick wraparound correctly for unsigned integers
        uint32_t elapsed_ms = ctx->get_tick() - ctx->start_tick;
        
        uint32_t timeout_threshold = (ctx->state == STATE_RECEIVING_HEADER) ?
                                     ctx->protocol->inter_byte_timeout_ms :
                                     ctx->protocol->frame_timeout_ms;

        if (elapsed_ms >= timeout_threshold) {
            if (ctx->callbacks.on_parse_error) {
                ctx->callbacks.on_parse_error(ctx->user_data, ERR_TIMEOUT);
            }
            // Discard the first byte of the potential frame and restart search.
            reset_parser(ctx, ctx->internal_state.frame.frame_start_offset + 1);
        }
    }

    // 2. Peek at the available data
    const uint8_t *data_ptr;
    size_t data_len = ctx->buffer_if->peek(ctx->buffer_if->handle, &data_ptr);

    // 3. Process the data block with the FSM if no reset was triggered by timeout
    if (ctx->bytes_to_consume == 0 && data_len > 0) {
        ctx->scan_offset = 0;
        while (ctx->scan_offset < data_len) {
            size_t last_scan_offset = ctx->scan_offset;
            
            fsm_step(ctx, data_ptr, data_len);

            if (ctx->bytes_to_consume > 0) break;
            if (ctx->scan_offset == last_scan_offset) break;
        }
        
        if (ctx->bytes_to_consume == 0) {
            ctx->bytes_to_consume = ctx->scan_offset;
        }
    }

    // 4. Consume the processed or junk data
    if (ctx->bytes_to_consume > 0) {
        ctx->buffer_if->consume(ctx->buffer_if->handle, ctx->bytes_to_consume);
        ctx->bytes_to_consume = 0; // Reset for the next run
    }
}
