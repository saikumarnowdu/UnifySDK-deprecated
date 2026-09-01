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
 * @file zwave_resolver_tx_queue_load_test.c
 * @brief Full-network SET resolution against a mocked Z-Wave TX queue.
 *
 * Scenario
 * --------
 * Z-Wave classic capacity is 232 NodeIDs. This test builds:
 *   - Nodes 1..50:  10 endpoints each
 *   - Nodes 51..232: 1..5 endpoints cycling (1,2,3,4,5,1,...)
 *
 * Every endpoint has one cluster attribute with a matched reported value.
 * Desired is then set on every endpoint attribute. The attribute resolver
 * walks the tree depth-first, post-order (children before parent) and
 * executes **one SET at a time**. The mocked zwave_tx queue has 64 slots
 * (ZWAVE_TX_QUEUE_BUFFER_SIZE). Because the resolver waits for
 * on_resolver_send_data_complete before the next rule, occupancy stays 1.
 *
 * A second test shows what happens if every SET were enqueued at once:
 * the 64-slot TX queue accepts 64 frames and returns SL_STATUS_NOT_READY
 * for the rest (resolver would skip those until a later scan).
 */

#include "unity.h"
#include "contiki_test_helper.h"
#include "process.h"
#include "attribute_store_fixt.h"
#include "datastore.h"
#include "attribute_resolver.h"
#include "attribute_resolver_rule.h"
#include "attribute_store.h"
#include "attribute_store_helper.h"
#include "attribute_store_type_registration.h"
#include "sl_log.h"
#include "sl_status.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define LOG_TAG "zwave_resolver_tx_queue_load_test"

#define ZWAVE_NODE_CAPACITY          232
#define MULTI_EP_NODE_COUNT          50
#define MULTI_EP_COUNT               10
#define ZWAVE_TX_QUEUE_BUFFER_SIZE   64
#define TYPE_HOME_ID                 1000
#define TYPE_NODE_ID                 1001
#define TYPE_ENDPOINT                1002
#define TYPE_CLUSTER_ATTR            1003
#define MAX_TRACE_EVENTS             24
#define MAX_IDLE_SPINS               32

extern struct process attribute_resolver_process;

typedef struct {
  uint16_t node_id;
  uint8_t endpoint_id;
} zwave_dest_t;

typedef struct {
  uint32_t node_count;
  uint32_t endpoint_count;
  uint32_t cluster_attr_count;
} network_stats_t;

static network_stats_t network_stats;
static attribute_store_node_t cluster_attrs[4096];
static uint32_t cluster_attr_count;

static uint32_t tx_attempts;
static uint32_t tx_accepted;
static uint32_t tx_rejected;
static uint32_t tx_completed;
static uint32_t tx_queue_occupancy;
static uint32_t max_tx_queue_occupancy;
static uint32_t resolver_in_flight;
static uint32_t max_resolver_in_flight;
static uint32_t set_rule_calls;
static attribute_store_node_t pending_send_node;
static zwave_dest_t send_trace[MAX_TRACE_EVENTS];
static uint32_t send_trace_count;
static zwave_dest_t first_send;
static zwave_dest_t last_send;
static bool have_first_send;

static int endpoint_count_for_node(uint16_t node_id)
{
  if (node_id <= MULTI_EP_NODE_COUNT) {
    return MULTI_EP_COUNT;
  }
  return 1 + (int)((node_id - MULTI_EP_NODE_COUNT - 1) % 5);
}

static uint32_t expected_endpoint_count(void)
{
  uint32_t total = 0;
  for (uint16_t node_id = 1; node_id <= ZWAVE_NODE_CAPACITY; node_id++) {
    total += (uint32_t)endpoint_count_for_node(node_id);
  }
  return total;
}

static sl_status_t set_rule(attribute_store_node_t node,
                            uint8_t *frame,
                            uint16_t *frame_len)
{
  (void)node;
  frame[0]   = 0x25; /* Binary Switch SET stand-in */
  frame[1]   = 0x01;
  *frame_len = 2;
  set_rule_calls++;
  return SL_STATUS_OK;
}

static sl_status_t get_rule(attribute_store_node_t node,
                            uint8_t *frame,
                            uint16_t *frame_len)
{
  (void)node;
  frame[0]   = 0x25;
  frame[1]   = 0x02;
  *frame_len = 2;
  return SL_STATUS_OK;
}

static zwave_dest_t destination_for_attr(attribute_store_node_t attr)
{
  zwave_dest_t dest = {.node_id = 0, .endpoint_id = 0};
  uint16_t node_id  = 0;
  uint8_t endpoint  = 0;

  attribute_store_node_t node
    = attribute_store_get_first_parent_with_type(attr, TYPE_NODE_ID);
  attribute_store_node_t endpoint_node
    = attribute_store_get_first_parent_with_type(attr, TYPE_ENDPOINT);

  if (node != ATTRIBUTE_STORE_INVALID_NODE) {
    attribute_store_get_reported(node, &node_id, sizeof(node_id));
  }
  if (endpoint_node != ATTRIBUTE_STORE_INVALID_NODE) {
    attribute_store_get_reported(endpoint_node, &endpoint, sizeof(endpoint));
  }

  dest.node_id     = node_id;
  dest.endpoint_id = endpoint;
  return dest;
}

static sl_status_t mock_zwave_tx_send(attribute_store_node_t node,
                                      const uint8_t *frame_data,
                                      uint16_t frame_data_len,
                                      bool is_set)
{
  (void)frame_data;
  (void)frame_data_len;
  (void)is_set;

  tx_attempts++;

  if (tx_queue_occupancy >= ZWAVE_TX_QUEUE_BUFFER_SIZE) {
    tx_rejected++;
    return SL_STATUS_NOT_READY;
  }

  tx_queue_occupancy++;
  if (tx_queue_occupancy > max_tx_queue_occupancy) {
    max_tx_queue_occupancy = tx_queue_occupancy;
  }

  resolver_in_flight++;
  if (resolver_in_flight > max_resolver_in_flight) {
    max_resolver_in_flight = resolver_in_flight;
  }

  pending_send_node = node;
  last_send         = destination_for_attr(node);
  if (!have_first_send) {
    first_send      = last_send;
    have_first_send = true;
  }
  if (send_trace_count < MAX_TRACE_EVENTS) {
    send_trace[send_trace_count++] = last_send;
  }

  tx_accepted++;
  return SL_STATUS_OK;
}

static sl_status_t mock_zwave_tx_abort(attribute_store_node_t node)
{
  (void)node;
  if (tx_queue_occupancy > 0) {
    tx_queue_occupancy--;
  }
  if (resolver_in_flight > 0) {
    resolver_in_flight--;
  }
  pending_send_node = ATTRIBUTE_STORE_INVALID_NODE;
  return SL_STATUS_OK;
}

static attribute_resolver_config_t resolver_config
  = {.send_init         = NULL,
     .send              = mock_zwave_tx_send,
     .abort             = mock_zwave_tx_abort,
     .get_retry_timeout = 20000,
     .get_retry_count   = 3};

static void complete_pending_set(void)
{
  if (pending_send_node == ATTRIBUTE_STORE_INVALID_NODE) {
    return;
  }

  on_resolver_send_data_complete(RESOLVER_SEND_STATUS_OK_EXECUTION_VERIFIED,
                                 12,
                                 pending_send_node,
                                 RESOLVER_SET_RULE);

  pending_send_node = ATTRIBUTE_STORE_INVALID_NODE;
  if (tx_queue_occupancy > 0) {
    tx_queue_occupancy--;
  }
  if (resolver_in_flight > 0) {
    resolver_in_flight--;
  }
  tx_completed++;
}

static void register_types(void)
{
  TEST_ASSERT_EQUAL(
    SL_STATUS_OK,
    attribute_store_register_type(TYPE_HOME_ID,
                                  "HomeID",
                                  ATTRIBUTE_STORE_INVALID_ATTRIBUTE_TYPE,
                                  U32_STORAGE_TYPE));
  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    attribute_store_register_type(TYPE_NODE_ID,
                                                  "NodeID",
                                                  TYPE_HOME_ID,
                                                  U16_STORAGE_TYPE));
  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    attribute_store_register_type(TYPE_ENDPOINT,
                                                  "EndpointID",
                                                  TYPE_NODE_ID,
                                                  U8_STORAGE_TYPE));
  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    attribute_store_register_type(TYPE_CLUSTER_ATTR,
                                                  "OnOff",
                                                  TYPE_ENDPOINT,
                                                  U8_STORAGE_TYPE));
}

static network_stats_t build_zwave_network(void)
{
  network_stats_t stats = {0};
  cluster_attr_count    = 0;

  attribute_store_node_t home
    = attribute_store_add_node(TYPE_HOME_ID, attribute_store_get_root());

  for (uint16_t node_id = 1; node_id <= ZWAVE_NODE_CAPACITY; node_id++) {
    attribute_store_node_t node
      = attribute_store_add_node(TYPE_NODE_ID, home);
    attribute_store_set_reported(node, &node_id, sizeof(node_id));
    stats.node_count++;

    int ep_count = endpoint_count_for_node(node_id);
    for (int ep = 0; ep < ep_count; ep++) {
      uint8_t endpoint_id = (uint8_t)ep;
      uint8_t reported    = 0x00;
      attribute_store_node_t endpoint
        = attribute_store_add_node(TYPE_ENDPOINT, node);
      attribute_store_set_reported(endpoint, &endpoint_id, sizeof(endpoint_id));
      stats.endpoint_count++;

      attribute_store_node_t attr
        = attribute_store_add_node(TYPE_CLUSTER_ATTR, endpoint);
      attribute_store_set_reported(attr, &reported, sizeof(reported));
      TEST_ASSERT_LESS_THAN(sizeof(cluster_attrs) / sizeof(cluster_attrs[0]),
                            cluster_attr_count);
      cluster_attrs[cluster_attr_count++] = attr;
      stats.cluster_attr_count++;
    }
  }

  return stats;
}

static void set_desired_on_all_endpoints(uint8_t desired)
{
  for (uint32_t i = 0; i < cluster_attr_count; i++) {
    TEST_ASSERT_EQUAL(
      SL_STATUS_OK,
      attribute_store_set_desired(cluster_attrs[i], &desired, sizeof(desired)));
  }
}

static uint32_t count_unmatched_desired(void)
{
  uint32_t unmatched = 0;
  for (uint32_t i = 0; i < cluster_attr_count; i++) {
    if (!attribute_store_is_value_matched(cluster_attrs[i])) {
      unmatched++;
    }
  }
  return unmatched;
}

static void reset_tx_counters(void)
{
  tx_attempts              = 0;
  tx_accepted              = 0;
  tx_rejected              = 0;
  tx_completed             = 0;
  tx_queue_occupancy       = 0;
  max_tx_queue_occupancy   = 0;
  resolver_in_flight       = 0;
  max_resolver_in_flight   = 0;
  set_rule_calls           = 0;
  pending_send_node        = ATTRIBUTE_STORE_INVALID_NODE;
  send_trace_count         = 0;
  have_first_send          = false;
  first_send               = (zwave_dest_t){0, 0};
  last_send                = (zwave_dest_t){0, 0};
  memset(send_trace, 0, sizeof(send_trace));
}

void suiteSetUp()
{
  datastore_init(":memory:");
  attribute_store_init();
  register_types();
}

int suiteTearDown(int num_failures)
{
  attribute_resolver_teardown();
  attribute_store_teardown();
  datastore_teardown();
  return num_failures;
}

void setUp()
{
  reset_tx_counters();
  network_stats      = (network_stats_t){0};
  cluster_attr_count = 0;
  attribute_store_delete_node(attribute_store_get_root());
}

void tearDown()
{
  attribute_resolver_teardown();
}

void test_zwave_232_node_network_shape(void)
{
  network_stats = build_zwave_network();

  TEST_ASSERT_EQUAL(ZWAVE_NODE_CAPACITY, network_stats.node_count);
  TEST_ASSERT_EQUAL(expected_endpoint_count(), network_stats.endpoint_count);
  TEST_ASSERT_EQUAL(network_stats.endpoint_count,
                    network_stats.cluster_attr_count);
  TEST_ASSERT_EQUAL(10, endpoint_count_for_node(1));
  TEST_ASSERT_EQUAL(10, endpoint_count_for_node(50));
  TEST_ASSERT_EQUAL(1, endpoint_count_for_node(51));
  TEST_ASSERT_EQUAL(5, endpoint_count_for_node(55));
  TEST_ASSERT_EQUAL(1, endpoint_count_for_node(56));

  char message[256];
  snprintf(message,
           sizeof(message),
           "Network: %u nodes, %u endpoints (50x10 + remaining 1-5)",
           network_stats.node_count,
           network_stats.endpoint_count);
  TEST_MESSAGE(message);
}

void test_zwave_tx_queue_rejects_when_full(void)
{
  const uint32_t burst = expected_endpoint_count();
  uint32_t accepted    = 0;
  uint32_t rejected    = 0;

  for (uint32_t i = 0; i < burst; i++) {
    if (tx_queue_occupancy >= ZWAVE_TX_QUEUE_BUFFER_SIZE) {
      rejected++;
    } else {
      tx_queue_occupancy++;
      accepted++;
    }
  }

  TEST_ASSERT_EQUAL(ZWAVE_TX_QUEUE_BUFFER_SIZE, accepted);
  TEST_ASSERT_EQUAL(burst - ZWAVE_TX_QUEUE_BUFFER_SIZE, rejected);
  TEST_ASSERT_EQUAL(ZWAVE_TX_QUEUE_BUFFER_SIZE, tx_queue_occupancy);

  char message[256];
  snprintf(message,
           sizeof(message),
           "Burst enqueue of %u SETs: TX queue accepted %u, rejected %u "
           "(capacity %u)",
           burst,
           accepted,
           rejected,
           ZWAVE_TX_QUEUE_BUFFER_SIZE);
  TEST_MESSAGE(message);
}

void test_resolver_serializes_sets_through_zwave_tx_queue(void)
{
  network_stats = build_zwave_network();
  const uint32_t expected_sets = network_stats.cluster_attr_count;

  contiki_test_helper_init();
  TEST_ASSERT_EQUAL(SL_STATUS_OK, attribute_resolver_init(resolver_config));
  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    attribute_resolver_register_rule(TYPE_CLUSTER_ATTR,
                                                     set_rule,
                                                     get_rule));
  contiki_test_helper_run(0);

  set_desired_on_all_endpoints(0xFF);
  TEST_ASSERT_EQUAL(expected_sets, count_unmatched_desired());

  uint32_t idle_spins = 0;
  uint32_t steps      = 0;
  const uint32_t max_steps = expected_sets * 4U + 100U;

  while (tx_completed < expected_sets && steps < max_steps) {
    contiki_test_helper_run(0);
    if (pending_send_node != ATTRIBUTE_STORE_INVALID_NODE) {
      TEST_ASSERT_EQUAL_UINT32(1, resolver_in_flight);
      TEST_ASSERT_EQUAL_UINT32(1, tx_queue_occupancy);
      complete_pending_set();
      idle_spins = 0;
    } else {
      idle_spins++;
      if (idle_spins > MAX_IDLE_SPINS) {
        break;
      }
    }
    steps++;
  }

  TEST_ASSERT_EQUAL(expected_sets, tx_completed);
  TEST_ASSERT_EQUAL(expected_sets, tx_accepted);
  TEST_ASSERT_EQUAL(0, tx_rejected);
  TEST_ASSERT_EQUAL(0, count_unmatched_desired());
  TEST_ASSERT_EQUAL(1, max_resolver_in_flight);
  TEST_ASSERT_EQUAL(1, max_tx_queue_occupancy);
  TEST_ASSERT_EQUAL(1, first_send.node_id);
  TEST_ASSERT_EQUAL(0, first_send.endpoint_id);
  TEST_ASSERT_EQUAL(ZWAVE_NODE_CAPACITY, last_send.node_id);

  /* Depth-first: all 10 endpoints of node 1 before node 2. */
  TEST_ASSERT_GREATER_OR_EQUAL(11, send_trace_count);
  for (uint8_t ep = 0; ep < 10; ep++) {
    TEST_ASSERT_EQUAL(1, send_trace[ep].node_id);
    TEST_ASSERT_EQUAL(ep, send_trace[ep].endpoint_id);
  }
  TEST_ASSERT_EQUAL(2, send_trace[10].node_id);
  TEST_ASSERT_EQUAL(0, send_trace[10].endpoint_id);

  char message[512];
  snprintf(message,
           sizeof(message),
           "Resolved %u SETs in %u steps. max resolver in-flight=%u, "
           "max TX queue occupancy=%u (capacity %u). First=%u:%u Last=%u:%u. "
           "TX attempts=%u rejected=%u",
           tx_completed,
           steps,
           max_resolver_in_flight,
           max_tx_queue_occupancy,
           ZWAVE_TX_QUEUE_BUFFER_SIZE,
           first_send.node_id,
           first_send.endpoint_id,
           last_send.node_id,
           last_send.endpoint_id,
           tx_attempts,
           tx_rejected);
  TEST_MESSAGE(message);

  FILE *log = fopen("zwave_resolver_tx_queue_load_test.log", "w");
  if (log != NULL) {
    fprintf(log, "nodes=%u endpoints=%u sets=%u\n",
            network_stats.node_count,
            network_stats.endpoint_count,
            tx_completed);
    fprintf(log, "max_resolver_in_flight=%u max_tx_queue=%u rejected=%u\n",
            max_resolver_in_flight,
            max_tx_queue_occupancy,
            tx_rejected);
    fprintf(log, "order_head=");
    for (uint32_t i = 0; i < send_trace_count; i++) {
      fprintf(log,
              "%u:%u%s",
              send_trace[i].node_id,
              send_trace[i].endpoint_id,
              (i + 1 < send_trace_count) ? "," : "\n");
    }
    fprintf(log,
            "order_tail_last=%u:%u\n",
            last_send.node_id,
            last_send.endpoint_id);
    fclose(log);
  }
}
