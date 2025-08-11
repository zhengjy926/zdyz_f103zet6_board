/**
 * @file        frame_parser.h
 * @author      Gemini
 * @version     1.1
 * @date        2024-07-17
 * @brief       Header file for the generic serial frame parser.
 * @details     This file contains all the public definitions, data structures,
 * and APIs for the frame parsing framework. It now uses a tick-based
 * timeout mechanism.
 */

#ifndef FRAME_PARSER_H
#define FRAME_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// --- Buffer Interface ---

/**
 * @brief Abstract interface for a byte buffer (like kfifo).
 * This allows the parser to work with any underlying circular buffer
 * implementation that can provide these functions.
 */
typedef struct buffer_if {
    /** Pointer to the specific buffer instance (e.g., a kfifo struct) */
    void *handle;

    /**
     * @brief Get the number of bytes currently in the buffer.
     * @param handle The buffer instance handle.
     * @return Number of used bytes in the buffer.
     */
    size_t (*len)(void *handle);

    /**
     * @brief Peek at the data in the buffer without removing it (Zero-Copy).
     * This is the core function for zero-copy parsing. It should return
     * a direct pointer to a contiguous block of data within the buffer.
     * @param handle The buffer instance handle.
     * @param ptr Output pointer that will be set to the start of the data block.
     * @return The length of the contiguous data block available for reading.
     * If the data wraps around in the circular buffer, this should return
     * the length of the first part (up to the end of the physical buffer).
     */
    size_t (*peek)(void *handle, const uint8_t **ptr);

    /**
     * @brief Consume (remove/discard) data from the front of the buffer.
     * This is called after a frame is successfully parsed or after junk
     * data is identified and needs to be skipped.
     * @param handle The buffer instance handle.
     * @param count Number of bytes to consume.
     */
    void (*consume)(void *handle, size_t count);

} buffer_if_t;

// --- Enumerations ---

/**
 * @brief Defines the type of frame structure.
 */
typedef enum {
    FRAME_TYPE_FIXED_LEN,     /**< Fixed length frames */
    FRAME_TYPE_LEN_PREFIX,    /**< Variable length with a length field */
    FRAME_TYPE_DELIMITER,     /**< Variable length, terminated by a delimiter (tail) */
} frame_type_t;

/**
 * @brief Defines the endianness of multi-byte fields within a frame.
 */
typedef enum {
    ENDIAN_LITTLE,            /**< Little-Endian (e.g., x86) */
    ENDIAN_BIG,               /**< Big-Endian (e.g., network byte order) */
} endianness_t;

/**
 * @brief Internal states of the Frame State Machine (FSM).
 */
typedef enum {
    STATE_SYNC_SEARCH,        /**< Searching for the first byte of the header */
    STATE_RECEIVING_HEADER,   /**< Receiving subsequent bytes of a multi-byte header */
    STATE_EXTRACT_LENGTH,     /**< For LEN_PREFIX frames, reading the length field */
    STATE_RECEIVING_PAYLOAD,  /**< Receiving the main body of the frame */
    STATE_VALIDATE_FRAME,     /**< All data received, performing final validation (tail, checksum) */
} parser_state_t;

/**
 * @brief Error codes reported by the parser.
 */
typedef enum {
    ERR_NONE,
    ERR_TIMEOUT,              /**< Frame reception timed out */
    ERR_INVALID_LENGTH,       /**< Length field value exceeds max_frame_len */
    ERR_BAD_TAIL,             /**< Frame tail mismatch */
    ERR_BAD_CHECKSUM,         /**< Checksum validation failed */
} error_code_t;


// --- Type Definitions ---

/**
 * @brief Function pointer for a checksum calculation algorithm.
 * @param data Pointer to the data to be checksummed.
 * @param len Length of the data.
 * @return The calculated checksum value (up to 32 bits).
 */
typedef uint32_t (*checksum_func_t)(const uint8_t *data, size_t len);

/**
 * @brief Function pointer to get the system's current tick count (e.g., in milliseconds).
 * @return Current system tick.
 */
typedef uint32_t (*get_tick_func_t)(void);

/**
 * @brief Structure to define a specific protocol's characteristics.
 * An instance of this struct is the "configuration" for the parser.
 */
typedef struct {
    // --- Basic Frame Structure ---
    frame_type_t frame_type;          /**< The type of frame (fixed, len_prefix, delimiter) */
    const uint8_t *header;            /**< Pointer to the header byte sequence */
    size_t header_len;                /**< Length of the header sequence */
    const uint8_t *tail;              /**< Pointer to the tail byte sequence */
    size_t tail_len;                  /**< Length of the tail sequence */

    // --- Frame-specific Parameters ---
    struct {
        size_t frame_total_len;       /**< For FIXED_LEN: the total length of the frame */
    } fixed_len_params;

    struct {
        size_t offset;                /**< For LEN_PREFIX: offset of the length field from frame start */
        size_t size;                  /**< For LEN_PREFIX: size of the length field (1, 2, or 4 bytes) */
        bool len_includes_all;        /**< For LEN_PREFIX: true if length field value is the total frame length */
    } len_prefix_params;

    // --- Checksum Parameters ---
    struct {
        checksum_func_t calc;         /**< Pointer to the checksum function. NULL if no checksum. */
        size_t size;                  /**< Size of the checksum field in bytes (e.g., 2 for CRC16) */
    } checksum_params;

    // --- Metadata ---
    endianness_t endianness;          /**< Endianness for multi-byte fields (e.g., length field) */
    size_t max_frame_len;             /**< Sanity check: maximum allowed frame length */
    uint32_t inter_byte_timeout_ms;   /**< Timeout for receiving subsequent header bytes */
    uint32_t frame_timeout_ms;        /**< Timeout for receiving a full frame after header is locked */

} protocol_def_t;

/**
 * @brief Structure for application-defined callback functions.
 */
typedef struct {
    /**
     * @brief Called when a complete, valid frame is received.
     * @param user_data A pointer to user-defined context.
     * @param payload Pointer to the payload data (inside the kfifo, zero-copy).
     * @param len Length of the payload.
     */
    void (*on_frame_received)(void *user_data, const uint8_t *payload, size_t len);

    /**
     * @brief Called when a parsing error occurs.
     * @param user_data A pointer to user-defined context.
     * @param error The specific error code.
     */
    void (*on_parse_error)(void *user_data, error_code_t error);
} callbacks_t;

/**
 * @brief The main parser context structure.
 * This holds the state and configuration for a single parsing stream.
 */
typedef struct {
    // --- Configuration (set at init) ---
    const protocol_def_t *protocol;   /**< Pointer to the protocol definition */
    buffer_if_t *buffer_if;           /**< Pointer to the buffer interface implementation */
    get_tick_func_t get_tick;         /**< Pointer to the system tick function */
    callbacks_t callbacks;            /**< Application callbacks */
    void *user_data;                  /**< App-specific data passed to callbacks */

    // --- FSM State ---
    parser_state_t state;             /**< The current state of the FSM */
    uint32_t start_tick;              /**< Timestamp when frame reception began */
    union {
        // State data for STATE_RECEIVING_HEADER
        struct {
            size_t matched_bytes;
        } sync;
        // State data for payload reception and validation
        struct {
            size_t frame_start_offset; // Start of frame in the current peek buffer
            size_t expected_len;       // Total expected frame length
        } frame;
    } internal_state;

    // --- Zero-Copy Buffer Management ---
    size_t scan_offset;               /**< Current read offset within the peeked buffer block */
    size_t bytes_to_consume;          /**< Bytes to be consumed from kfifo in the next step */

} parser_context_t;


// --- Public API ---

/**
 * @brief Initializes a parser context.
 * @param ctx Pointer to the parser_context_t to initialize.
 * @param protocol Pointer to the protocol definition.
 * @param buffer_if Pointer to the buffer interface.
 * @param get_tick Pointer to the system tick function. Can be NULL if no timeout is needed.
 * @param callbacks Struct with application callbacks.
 * @param user_data Optional pointer to user data.
 */
void parser_init(
    parser_context_t *ctx,
    const protocol_def_t *protocol,
    buffer_if_t *buffer_if,
    get_tick_func_t get_tick,
    callbacks_t callbacks,
    void *user_data
);

/**
 * @brief Processes the data in the buffer.
 * This function should be called periodically in the application's main loop.
 * It drives the state machine and performs the parsing.
 * @param ctx Pointer to the parser context.
 */
void parser_process(parser_context_t *ctx);

#endif // FRAME_PARSER_H
