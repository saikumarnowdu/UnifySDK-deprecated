/**
 * @file zwave_python_harness.h
 * @brief Stable C ABI for Python ctypes bindings — direct Z-Wave Serial API / zwapi
 *        testing without UCL or MQTT.
 */
#ifndef ZWAVE_PYTHON_HARNESS_H
#define ZWAVE_PYTHON_HARNESS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZWAVE_HARNESS_MAX_PAYLOAD 255
#define ZWAVE_HARNESS_MAX_ERROR   256

/** Opaque handle returned by zwave_harness_open. */
typedef struct zwave_harness zwave_harness_t;

/**
 * Open serial port and initialize zwapi.
 * @param serial_port e.g. "/dev/ttyUSB0"
 * @returns handle or NULL on failure (see zwave_harness_last_error)
 */
zwave_harness_t *zwave_harness_open(const char *serial_port);

/** Close port and release zwapi. */
void zwave_harness_close(zwave_harness_t *handle);

/**
 * Drive zwapi receive processing. Call frequently from Python loop.
 * @returns true if more frames remain in the RX queue.
 */
bool zwave_harness_poll(zwave_harness_t *handle);

/** Last error message for the most recent failing call on this handle. */
const char *zwave_harness_last_error(zwave_harness_t *handle);

/** Set sl_log verbosity: 0=debug .. 4=critical. */
void zwave_harness_set_log_level(int level);

/**
 * Raw Serial API request/response (FUNC_ID + payload).
 * @param func_id Serial API function id
 * @param payload request bytes (may be NULL if payload_len is 0)
 * @param response_out buffer for response payload (func byte + data)
 * @param response_len_inout max size on input, actual length on output
 */
int zwave_harness_send_frame(zwave_harness_t *handle,
                             uint8_t func_id,
                             const uint8_t *payload,
                             uint8_t payload_len,
                             uint8_t *response_out,
                             uint8_t *response_len_inout);

/** FUNC_ID_ZW_GET_VERSION — fills major/minor on success. */
int zwave_harness_get_version(zwave_harness_t *handle,
                              uint8_t *major,
                              uint8_t *minor);

/** zwapi_get_protocol_version — writes human-readable string to out. */
int zwave_harness_get_protocol_version_string(zwave_harness_t *handle,
                                              char *out,
                                              int out_len);

/** zwapi_get_library_type — controller/static/slave etc. */
int zwave_harness_get_library_type(zwave_harness_t *handle, uint8_t *library_type);

/** zwapi_send_nop */
int zwave_harness_send_nop(zwave_harness_t *handle,
                           uint8_t node_id,
                           uint8_t tx_options);

/** zwapi_send_data — payload is full Z-Wave frame (command class first byte). */
int zwave_harness_send_data(zwave_harness_t *handle,
                            uint8_t node_id,
                            const uint8_t *data,
                            uint8_t data_len,
                            uint8_t tx_options);

/**
 * Last application frame received (from application_command_handler callback).
 * @param rx_status_out optional receive status flags
 * @param source_node_out optional source node id
 */
int zwave_harness_get_last_app_frame(zwave_harness_t *handle,
                                     uint8_t *frame_out,
                                     uint8_t *frame_len_inout,
                                     uint8_t *rx_status_out,
                                     uint8_t *source_node_out);

/** Last transmit complete status from send_data/nop callback (0xff if none). */
int zwave_harness_get_last_tx_status(zwave_harness_t *handle, uint8_t *tx_status);

#ifdef __cplusplus
}
#endif

#endif /* ZWAVE_PYTHON_HARNESS_H */
