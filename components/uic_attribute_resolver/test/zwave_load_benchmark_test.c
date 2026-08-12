/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
/**
 * @file zwave_load_benchmark_test.c
 * @brief Programmatic load tests for a 200-node Z-Wave-shaped attribute tree.
 *
 * Simulates UNID -> endpoint (1-10) -> cluster attribute hierarchy and
 * exercises TX queue backpressure behaviour without NCP hardware.
 */

#include "unity.h"
#include "contiki_test_helper.h"
#include "process.h"
#include "attribute_store_fixt.h"
#include "datastore_fixt.h"
#include "attribute_resolver.h"
#include "attribute_resolver_rule.h"
#include "sl_log.h"
#include "attribute_store.h"
#include "attribute_store_type_registration.h"
#include "attribute_store_helper.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define LOG_TAG "zwave_load_benchmark_test"

#define DB_FILENAME "zwave_load_benchmark_test.db"

#define LOAD_TEST_NODE_COUNT      200
#define LOAD_TEST_MAX_ENDPOINTS   10
#define LOAD_TEST_TX_QUEUE_DEPTH  32

#define TYPE_HOME_ID      1000
#define TYPE_UNID         1001
#define TYPE_ENDPOINT     1002
#define TYPE_CLUSTER_ATTR 1003

#define MAX_NETWORK_BUILD_TIME_SEC  5.0
#define MAX_RESOLVER_SCAN_TIME_SEC  60.0

#define TX_BACKPRESSURE_ATTEMPTS 1000

typedef enum {
  RESOLVER_NEXT_EVENT,
} attribute_resolver_worker_event_t;

extern struct process attribute_resolver_process;

typedef struct {
  uint32_t unid_count;
  uint32_t endpoint_count;
  uint32_t cluster_attr_count;
} zwave_network_stats_t;

static int tx_queue_depth      = 0;
static int tx_in_flight        = 0;
static uint32_t tx_attempts    = 0;
static uint32_t tx_dropped     = 0;
static uint32_t tx_completed   = 0;
static attribute_store_node_t pending_send_node = ATTRIBUTE_STORE_INVALID_NODE;

static zwave_network_stats_t network_stats = {0};

static int endpoint_count_for_unid(uint32_t unid_index)
{
  return (int)((unid_index * 7U + 3U) % LOAD_TEST_MAX_ENDPOINTS) + 1;
}

static sl_status_t load_test_get_rule(attribute_store_node_t node,
                                      uint8_t *frame,
                                      uint16_t *frame_len)
{
  (void)node;
  frame[0]    = 0x01;
  *frame_len  = 1;
  return SL_STATUS_OK;
}

static void complete_pending_send(void)
{
  if (pending_send_node == ATTRIBUTE_STORE_INVALID_NODE) {
    return;
  }

  uint8_t reported_value = 0xFF;
  attribute_store_set_reported(pending_send_node,
                               &reported_value,
                               sizeof(reported_value));
  on_resolver_send_data_complete(RESOLVER_SEND_STATUS_OK_EXECUTION_VERIFIED,
                               10,
                               pending_send_node,
                               RESOLVER_GET_RULE);
  pending_send_node = ATTRIBUTE_STORE_INVALID_NODE;
  if (tx_in_flight > 0) {
    tx_in_flight--;
  }
  tx_completed++;
}

static sl_status_t load_test_send(attribute_store_node_t node,
                                  const uint8_t *frame_data,
                                  uint16_t frame_data_len,
                                  bool is_set)
{
  (void)frame_data;
  (void)frame_data_len;
  (void)is_set;

  tx_attempts++;

  if (tx_in_flight >= LOAD_TEST_TX_QUEUE_DEPTH) {
    tx_dropped++;
    return SL_STATUS_NOT_READY;
  }

  if (pending_send_node != ATTRIBUTE_STORE_INVALID_NODE) {
    tx_dropped++;
    return SL_STATUS_NOT_READY;
  }

  tx_in_flight++;
  pending_send_node = node;
  return SL_STATUS_OK;
}

static sl_status_t load_test_abort(attribute_store_node_t node)
{
  (void)node;
  if (tx_in_flight > 0) {
    tx_in_flight--;
  }
  return SL_STATUS_OK;
}

static attribute_resolver_config_t load_test_resolver_config
  = {.send_init         = NULL,
     .send              = load_test_send,
     .abort             = load_test_abort,
     .get_retry_timeout = 20000,
     .get_retry_count   = 3};

static void register_load_test_attribute_types(void)
{
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    attribute_store_register_type(TYPE_HOME_ID,
                                  "HomeID",
                                  ATTRIBUTE_STORE_INVALID_ATTRIBUTE_TYPE,
                                  U32_STORAGE_TYPE));
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    attribute_store_register_type(TYPE_UNID,
                                  "UNID",
                                  TYPE_HOME_ID,
                                  C_STRING_STORAGE_TYPE));
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    attribute_store_register_type(TYPE_ENDPOINT,
                                  "EndpointID",
                                  TYPE_UNID,
                                  U8_STORAGE_TYPE));
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    attribute_store_register_type(TYPE_CLUSTER_ATTR,
                                  "ClusterAttribute",
                                  TYPE_ENDPOINT,
                                  U8_STORAGE_TYPE));
}

static zwave_network_stats_t build_zwave_load_network(uint32_t unid_count)
{
  zwave_network_stats_t stats = {0};
  attribute_store_node_t root_node = attribute_store_get_root();
  attribute_store_node_t home_node
    = attribute_store_add_node(TYPE_HOME_ID, root_node);

  for (uint32_t unid_index = 1; unid_index <= unid_count; unid_index++) {
    char unid_value[16];
    snprintf(unid_value, sizeof(unid_value), "zw-%04u", unid_index);

    attribute_store_node_t unid_node
      = attribute_store_add_node(TYPE_UNID, home_node);
    attribute_store_set_reported_string(unid_node, unid_value);
    stats.unid_count++;

    int endpoint_count = endpoint_count_for_unid(unid_index);
    for (int endpoint_id = 0; endpoint_id < endpoint_count; endpoint_id++) {
      attribute_store_node_t endpoint_node
        = attribute_store_add_node(TYPE_ENDPOINT, unid_node);
      uint8_t endpoint_value = (uint8_t)endpoint_id;
      attribute_store_set_reported(endpoint_node,
                                   &endpoint_value,
                                   sizeof(endpoint_value));
      stats.endpoint_count++;

      attribute_store_node_t cluster_attr_node
        = attribute_store_add_node(TYPE_CLUSTER_ATTR, endpoint_node);
      stats.cluster_attr_count++;
      (void)cluster_attr_node;
    }
  }

  return stats;
}

static sl_status_t simulate_tx_queue_send(void)
{
  tx_attempts++;

  if (tx_queue_depth >= LOAD_TEST_TX_QUEUE_DEPTH) {
    tx_dropped++;
    return SL_STATUS_NOT_READY;
  }

  tx_queue_depth++;
  return SL_STATUS_OK;
}

static void simulate_tx_queue_complete(void)
{
  if (tx_queue_depth > 0) {
    tx_queue_depth--;
    tx_completed++;
  }
}

void suiteSetUp()
{
  datastore_fixt_setup(DB_FILENAME);
  attribute_store_init();
  register_load_test_attribute_types();
}

int suiteTearDown(int num_failures)
{
  attribute_resolver_teardown();
  attribute_store_teardown();
  datastore_fixt_teardown();
  remove(DB_FILENAME);
  return num_failures;
}

void setUp()
{
  tx_queue_depth   = 0;
  tx_in_flight     = 0;
  tx_attempts      = 0;
  tx_dropped       = 0;
  tx_completed     = 0;
  pending_send_node = ATTRIBUTE_STORE_INVALID_NODE;
  network_stats    = (zwave_network_stats_t){0};
  attribute_store_delete_node(attribute_store_get_root());
}

void test_zwave_load_network_build_under_threshold(void)
{
  clock_t start_time = clock();

  network_stats = build_zwave_load_network(LOAD_TEST_NODE_COUNT);

  double elapsed_time = ((double)(clock() - start_time)) / CLOCKS_PER_SEC;

  TEST_ASSERT_EQUAL(LOAD_TEST_NODE_COUNT, network_stats.unid_count);
  TEST_ASSERT_GREATER_THAN(LOAD_TEST_NODE_COUNT, network_stats.endpoint_count);
  TEST_ASSERT_EQUAL(network_stats.endpoint_count, network_stats.cluster_attr_count);
  TEST_ASSERT_LESS_THAN(MAX_NETWORK_BUILD_TIME_SEC, elapsed_time);

  char message[256];
  snprintf(message,
           sizeof(message),
           "Built %u UNIDs, %u endpoints, %u cluster attrs in %.3f s",
           network_stats.unid_count,
           network_stats.endpoint_count,
           network_stats.cluster_attr_count,
           elapsed_time);
  TEST_MESSAGE(message);
}

void test_zwave_load_tx_queue_backpressure(void)
{
  const uint32_t attempts = TX_BACKPRESSURE_ATTEMPTS;

  for (uint32_t i = 0; i < attempts; i++) {
    sl_status_t status = simulate_tx_queue_send();
    if (status == SL_STATUS_OK && (i % 3U) == 0U) {
      simulate_tx_queue_complete();
    }
  }

  while (tx_queue_depth > 0) {
    simulate_tx_queue_complete();
  }

  TEST_ASSERT_EQUAL(attempts, tx_attempts);
  TEST_ASSERT_GREATER_THAN(0, tx_dropped);
  TEST_ASSERT_GREATER_THAN(0, tx_completed);

  char message[256];
  snprintf(message,
           sizeof(message),
           "TX queue: %u attempts, %u dropped, %u completed",
           tx_attempts,
           tx_dropped,
           tx_completed);
  TEST_MESSAGE(message);
}

void test_zwave_load_resolver_throughput(void)
{
  network_stats = build_zwave_load_network(LOAD_TEST_NODE_COUNT);

  contiki_test_helper_init();
  clock_t start_time = clock();

  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    attribute_resolver_init(load_test_resolver_config));
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    attribute_resolver_register_rule(TYPE_CLUSTER_ATTR,
                                     NULL,
                                     load_test_get_rule));
  contiki_test_helper_run(0);

  uint32_t resolver_steps = 0;
  const uint32_t max_steps = network_stats.cluster_attr_count + 10U;

  while (resolver_steps < max_steps) {
    if (tx_completed >= network_stats.cluster_attr_count) {
      break;
    }

    process_post(&attribute_resolver_process, RESOLVER_NEXT_EVENT, NULL);
    contiki_test_helper_run(0);
    complete_pending_send();
    resolver_steps++;
  }

  double elapsed_time = ((double)(clock() - start_time)) / CLOCKS_PER_SEC;

  TEST_ASSERT_EQUAL(network_stats.cluster_attr_count, tx_completed);
  TEST_ASSERT_EQUAL(0, tx_dropped);
  TEST_ASSERT_LESS_THAN(MAX_RESOLVER_SCAN_TIME_SEC, elapsed_time);

  char message[256];
  snprintf(message,
           sizeof(message),
           "Resolved %u cluster attrs in %u steps, %.3f s, %u TX attempts",
           tx_completed,
           resolver_steps,
           elapsed_time,
           tx_attempts);
  TEST_MESSAGE(message);

  attribute_resolver_teardown();
}
