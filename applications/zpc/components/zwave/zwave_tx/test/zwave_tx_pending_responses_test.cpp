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

#include "zwave_tx_pending_responses.hpp"

#include "unity.h"
#include "sl_status.h"

void test_pending_responses_tracks_nodes_independently()
{
  zwave_tx_pending_responses pending;

  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    pending.add((zwave_tx_session_id_t)0x1, 4, 1000));
  TEST_ASSERT_EQUAL(SL_STATUS_OK,
                    pending.add((zwave_tx_session_id_t)0x2, 7, 2000));

  TEST_ASSERT_TRUE(pending.is_node_waiting(4));
  TEST_ASSERT_TRUE(pending.is_node_waiting(7));
  TEST_ASSERT_FALSE(pending.is_node_waiting(5));
  TEST_ASSERT_EQUAL(2, pending.size());
  TEST_ASSERT_EQUAL(1000, pending.earliest_deadline());
}

void test_pending_responses_expire_only_due_items()
{
  zwave_tx_pending_responses pending;

  pending.add((zwave_tx_session_id_t)0x1, 4, 100);
  pending.add((zwave_tx_session_id_t)0x2, 7, 500);

  size_t expired_count = 0;
  pending.for_each_expired(200, [&](const auto &item) {
    TEST_ASSERT_EQUAL(4, item.node_id);
    TEST_ASSERT_EQUAL((zwave_tx_session_id_t)0x1, item.session_id);
    expired_count++;
  });

  TEST_ASSERT_EQUAL(1, expired_count);
  TEST_ASSERT_EQUAL(1, pending.size());
  TEST_ASSERT_TRUE(pending.is_node_waiting(7));
  TEST_ASSERT_FALSE(pending.is_node_waiting(4));
}

void test_pending_responses_remove_clears_node_block()
{
  zwave_tx_pending_responses pending;

  pending.add((zwave_tx_session_id_t)0x1, 4, 1000);
  TEST_ASSERT_TRUE(pending.is_node_waiting(4));

  TEST_ASSERT_EQUAL(SL_STATUS_OK, pending.remove((zwave_tx_session_id_t)0x1));
  TEST_ASSERT_FALSE(pending.is_node_waiting(4));
  TEST_ASSERT_TRUE(pending.empty());
}

#ifdef ZWAVE_TX_STANDALONE_TEST
int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_pending_responses_tracks_nodes_independently);
  RUN_TEST(test_pending_responses_expire_only_due_items);
  RUN_TEST(test_pending_responses_remove_clears_node_block);
  return UNITY_END();
}
#endif
