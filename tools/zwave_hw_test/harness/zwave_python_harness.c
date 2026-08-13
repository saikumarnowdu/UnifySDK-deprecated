/**
 * @file zwave_python_harness.c
 * @brief ctypes-friendly wrapper around zwapi for real-hardware automation.
 */
#include "zwave_python_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sl_log.h"
#include "zwave_api.h"
#include "zwapi_session.h"

struct zwave_harness {
  char last_error[ZWAVE_HARNESS_MAX_ERROR];
  uint8_t last_app_frame[ZWAVE_HARNESS_MAX_PAYLOAD];
  uint8_t last_app_frame_len;
  uint8_t last_app_rx_status;
  uint8_t last_app_source_node;
  uint8_t last_tx_status;
  bool has_tx_status;
};

static zwave_harness_t *active_harness = NULL;

static void set_error(zwave_harness_t *handle, const char *msg)
{
  if (handle == NULL) {
    return;
  }
  snprintf(handle->last_error, ZWAVE_HARNESS_MAX_ERROR, "%s", msg != NULL ? msg : "");
}

static void clear_error(zwave_harness_t *handle)
{
  set_error(handle, "");
}

const char *zwave_harness_last_error(zwave_harness_t *handle)
{
  if (handle == NULL) {
    return "invalid handle";
  }
  return handle->last_error;
}

void zwave_harness_set_log_level(int level)
{
  if (level < SL_LOG_DEBUG) {
    level = SL_LOG_DEBUG;
  }
  if (level > SL_LOG_CRITICAL) {
    level = SL_LOG_CRITICAL;
  }
  sl_log_set_level((sl_log_level_t)level);
}

static void harness_app_command_handler(uint8_t rx_status,
                                        zwave_node_id_t destination_node_id,
                                        zwave_node_id_t source_node_id,
                                        const uint8_t *received_frame,
                                        uint8_t received_frame_length,
                                        int8_t rssi_value)
{
  (void)destination_node_id;
  (void)rssi_value;
  zwave_harness_t *handle = active_harness;
  if (handle == NULL || received_frame == NULL) {
    return;
  }
  if (received_frame_length > ZWAVE_HARNESS_MAX_PAYLOAD) {
    received_frame_length = ZWAVE_HARNESS_MAX_PAYLOAD;
  }
  memcpy(handle->last_app_frame, received_frame, received_frame_length);
  handle->last_app_frame_len    = received_frame_length;
  handle->last_app_rx_status    = rx_status;
  handle->last_app_source_node  = (uint8_t)source_node_id;
}

static void harness_tx_callback(uint8_t tx_status, zwapi_tx_report_t *report)
{
  (void)report;
  zwave_harness_t *handle = active_harness;
  if (handle == NULL) {
    return;
  }
  handle->last_tx_status = tx_status;
  handle->has_tx_status  = true;
}


zwave_harness_t *zwave_harness_open(const char *serial_port)
{
  if (serial_port == NULL) {
    return NULL;
  }

  zwave_harness_t *handle = calloc(1, sizeof(zwave_harness_t));
  if (handle == NULL) {
    return NULL;
  }

  zwapi_callbacks_t callbacks = {0};
  callbacks.application_command_handler = harness_app_command_handler;

  active_harness = handle;

  int serial_fd = -1;
  sl_status_t status = zwapi_init(serial_port, &serial_fd, &callbacks);
  if (status != SL_STATUS_OK) {
    set_error(handle, "zwapi_init failed");
    free(handle);
    return NULL;
  }

  clear_error(handle);
  return handle;
}

void zwave_harness_close(zwave_harness_t *handle)
{
  if (handle == NULL) {
    return;
  }
  zwapi_destroy();
  active_harness = NULL;
  free(handle);
}

bool zwave_harness_poll(zwave_harness_t *handle)
{
  if (handle == NULL) {
    return false;
  }
  return zwapi_poll();
}

int zwave_harness_send_frame(zwave_harness_t *handle,
                             uint8_t func_id,
                             const uint8_t *payload,
                             uint8_t payload_len,
                             uint8_t *response_out,
                             uint8_t *response_len_inout)
{
  if (handle == NULL || response_out == NULL || response_len_inout == NULL) {
    return -1;
  }

  uint8_t response_len = *response_len_inout;
  sl_status_t status =
    zwapi_session_send_frame_with_response(func_id,
                                           payload,
                                           payload_len,
                                           response_out,
                                           &response_len);
  if (status != SL_STATUS_OK) {
    set_error(handle, "zwapi_session_send_frame_with_response failed");
    return -1;
  }
  *response_len_inout = response_len;
  clear_error(handle);
  return 0;
}

int zwave_harness_get_version(zwave_harness_t *handle,
                              uint8_t *major,
                              uint8_t *minor)
{
  if (handle == NULL || major == NULL || minor == NULL) {
    return -1;
  }
  zwapi_get_app_version(major, minor);
  clear_error(handle);
  return 0;
}

int zwave_harness_get_protocol_version_string(zwave_harness_t *handle,
                                              char *out,
                                              int out_len)
{
  if (handle == NULL || out == NULL || out_len <= 0) {
    return -1;
  }

  zwapi_protocol_version_information_t info = {0};
  if (zwapi_get_protocol_version(&info) != SL_STATUS_OK) {
    set_error(handle, "zwapi_get_protocol_version failed");
    return -1;
  }

  snprintf(out,
           (size_t)out_len,
           "%u.%u.%u build %u",
           info.major_version,
           info.minor_version,
           info.revision_version,
           info.build_number);
  clear_error(handle);
  return 0;
}

int zwave_harness_get_library_type(zwave_harness_t *handle, uint8_t *library_type)
{
  if (handle == NULL || library_type == NULL) {
    return -1;
  }
  *library_type = (uint8_t)zwapi_get_library_type();
  clear_error(handle);
  return 0;
}

int zwave_harness_send_nop(zwave_harness_t *handle,
                           uint8_t node_id,
                           uint8_t tx_options)
{
  if (handle == NULL) {
    return -1;
  }
  handle->has_tx_status = false;
  sl_status_t status =
    zwapi_send_nop((zwave_node_id_t)node_id, tx_options, harness_tx_callback);
  if (status != SL_STATUS_OK) {
    set_error(handle, "zwapi_send_nop rejected");
    return -1;
  }
  clear_error(handle);
  return 0;
}

int zwave_harness_send_data(zwave_harness_t *handle,
                            uint8_t node_id,
                            const uint8_t *data,
                            uint8_t data_len,
                            uint8_t tx_options)
{
  if (handle == NULL || data == NULL || data_len == 0) {
    return -1;
  }
  handle->has_tx_status = false;
  sl_status_t status = zwapi_send_data((zwave_node_id_t)node_id,
                                       data,
                                       data_len,
                                       tx_options,
                                       harness_tx_callback);
  if (status != SL_STATUS_OK) {
    set_error(handle, "zwapi_send_data rejected");
    return -1;
  }
  clear_error(handle);
  return 0;
}

int zwave_harness_get_last_app_frame(zwave_harness_t *handle,
                                     uint8_t *frame_out,
                                     uint8_t *frame_len_inout,
                                     uint8_t *rx_status_out,
                                     uint8_t *source_node_out)
{
  if (handle == NULL || frame_out == NULL || frame_len_inout == NULL) {
    return -1;
  }
  if (handle->last_app_frame_len == 0) {
    set_error(handle, "no application frame received yet");
    return -1;
  }
  if (*frame_len_inout < handle->last_app_frame_len) {
    set_error(handle, "output buffer too small");
    return -1;
  }
  memcpy(frame_out, handle->last_app_frame, handle->last_app_frame_len);
  *frame_len_inout = handle->last_app_frame_len;
  if (rx_status_out != NULL) {
    *rx_status_out = handle->last_app_rx_status;
  }
  if (source_node_out != NULL) {
    *source_node_out = handle->last_app_source_node;
  }
  clear_error(handle);
  return 0;
}

int zwave_harness_get_last_tx_status(zwave_harness_t *handle, uint8_t *tx_status)
{
  if (handle == NULL || tx_status == NULL) {
    return -1;
  }
  if (!handle->has_tx_status) {
    set_error(handle, "no transmit status yet");
    return -1;
  }
  *tx_status = handle->last_tx_status;
  clear_error(handle);
  return 0;
}
