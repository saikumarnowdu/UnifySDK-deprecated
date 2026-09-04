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

#include "unity.h"
#include "zwave_tx_queue.hpp"
#include "zwave_tx_route_cache.h"

#include <cstdint>

static zwave_tx_queue_element_t make_singlecast(uint32_t qos,
                                                zwave_node_id_t node_id,
                                                clock_time_t timestamp)
{
  zwave_tx_queue_element_t element = {};
  element.options.qos_priority     = qos;
  element.connection_info.remote.node_id      = node_id;
  element.connection_info.remote.endpoint_id  = 0;
  element.connection_info.remote.is_multicast = false;
  element.queue_timestamp                     = timestamp;
  return element;
}

void setUp()
{
  zwave_tx_route_cache_init();
}

void test_queue_qos_compare_groups_same_node()
{
  queue_element_qos_compare compare;

  const zwave_tx_queue_element_t node_a_first = make_singlecast(10, 4, 100);
  const zwave_tx_queue_element_t node_b       = make_singlecast(10, 5, 50);
  const zwave_tx_queue_element_t node_a_second = make_singlecast(10, 4, 200);

  TEST_ASSERT_TRUE(compare(node_a_first, node_b));
  TEST_ASSERT_TRUE(compare(node_a_first, node_a_second));
}

void test_queue_send_compare_uses_link_score_before_node_id()
{
  // Same QoS: a strong direct node should be sent before a routed FL node,
  // even if the routed NodeID is smaller.
  zwapi_tx_report_t direct = {};
  direct.ack_rssi          = -45;
  direct.last_route_speed  = ZWAVE_100_KBITS_S;
  zwave_tx_route_cache_update_from_tx_report(12, &direct);

  zwapi_tx_report_t routed = {};
  routed.number_of_repeaters = 4;
  routed.ack_rssi            = -95;
  routed.last_route_speed    = ZWAVE_9_6_KBITS_S;
  routed.beam_1000ms         = true;
  zwave_tx_route_cache_update_from_tx_report(3, &routed);

  queue_element_send_compare compare;
  const zwave_tx_queue_element_t fast_node  = make_singlecast(10, 12, 200);
  const zwave_tx_queue_element_t slow_node  = make_singlecast(10, 3, 50);

  TEST_ASSERT_TRUE(zwave_tx_route_cache_link_score(12)
                   > zwave_tx_route_cache_link_score(3));
  TEST_ASSERT_TRUE(compare(fast_node, slow_node));
  TEST_ASSERT_FALSE(compare(slow_node, fast_node));
}

void test_queue_send_compare_qos_still_wins_over_link_score()
{
  zwapi_tx_report_t direct = {};
  direct.last_route_speed  = ZWAVE_100_KBITS_S;
  zwave_tx_route_cache_update_from_tx_report(12, &direct);

  queue_element_send_compare compare;
  const zwave_tx_queue_element_t high_qos_unknown
    = make_singlecast(0xFFFF, 99, 0);
  const zwave_tx_queue_element_t low_qos_direct = make_singlecast(10, 12, 0);

  TEST_ASSERT_TRUE(compare(high_qos_unknown, low_qos_direct));
}

#ifdef ZWAVE_TX_STANDALONE_TEST
int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_queue_qos_compare_groups_same_node);
  RUN_TEST(test_queue_send_compare_uses_link_score_before_node_id);
  RUN_TEST(test_queue_send_compare_qos_still_wins_over_link_score);
  return UNITY_END();
}
#endif
