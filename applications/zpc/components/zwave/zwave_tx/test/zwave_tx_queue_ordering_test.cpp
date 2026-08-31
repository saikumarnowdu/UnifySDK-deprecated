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

#include <cstdint>

struct test_queue_element {
  uint32_t qos_priority;
  uint16_t node_id;
  uint8_t endpoint_id;
  uint32_t queue_timestamp;
};

struct test_queue_element_qos_compare {
  bool operator()(const test_queue_element &lhs,
                  const test_queue_element &rhs) const
  {
    if (lhs.qos_priority != rhs.qos_priority) {
      return lhs.qos_priority > rhs.qos_priority;
    }
    if (lhs.node_id != rhs.node_id) {
      return lhs.node_id < rhs.node_id;
    }
    if (lhs.endpoint_id != rhs.endpoint_id) {
      return lhs.endpoint_id < rhs.endpoint_id;
    }
    return lhs.queue_timestamp < rhs.queue_timestamp;
  }
};

void test_queue_qos_compare_groups_same_node()
{
  test_queue_element_qos_compare compare;

  const test_queue_element node_a_first
    = {.qos_priority = 10, .node_id = 4, .endpoint_id = 0, .queue_timestamp = 100};
  const test_queue_element node_b
    = {.qos_priority = 10, .node_id = 5, .endpoint_id = 0, .queue_timestamp = 50};
  const test_queue_element node_a_second
    = {.qos_priority = 10, .node_id = 4, .endpoint_id = 0, .queue_timestamp = 200};

  TEST_ASSERT_TRUE(compare(node_a_first, node_b));
  TEST_ASSERT_TRUE(compare(node_a_first, node_a_second));
}

#ifdef ZWAVE_TX_STANDALONE_TEST
int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_queue_qos_compare_groups_same_node);
  return UNITY_END();
}
#endif
