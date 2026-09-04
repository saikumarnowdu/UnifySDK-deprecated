/******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "zwave_tx_route_cache.h"

/// Setup the test suite (called once before all test_xxx functions are called)
void suiteSetUp() {}

/// Teardown the test suite (called once after all test_xxx functions are called)
int suiteTearDown(int num_failures)
{
  return num_failures;
}

/// Called before each and every test
void setUp()
{
  zwave_tx_route_cache_init();
}

void test_zwave_tx_route_cache_set_and_get()
{
  for (zwave_node_id_t i = 0; i < ZW_LR_MAX_NODE_ID; i++) {
    TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(i));
  }

  // Set the number of repeaters for a few nodes
  zwave_tx_route_cache_set_number_of_repeaters(10, 2);
  zwave_tx_route_cache_set_number_of_repeaters(11, 3);
  zwave_tx_route_cache_set_number_of_repeaters(15, 5);
  zwave_tx_route_cache_set_number_of_repeaters(20, 0);
  zwave_tx_route_cache_set_number_of_repeaters(21, 1);

  // Verify that it still matches
  TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(0));
  TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(1));
  TEST_ASSERT_EQUAL(2, zwave_tx_route_cache_get_number_of_repeaters(10));
  TEST_ASSERT_EQUAL(3, zwave_tx_route_cache_get_number_of_repeaters(11));
  TEST_ASSERT_EQUAL(5, zwave_tx_route_cache_get_number_of_repeaters(15));
  TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(20));
  TEST_ASSERT_EQUAL(1, zwave_tx_route_cache_get_number_of_repeaters(21));

  // Modify a few values
  zwave_tx_route_cache_set_number_of_repeaters(11, 5);
  zwave_tx_route_cache_set_number_of_repeaters(15, 0);
  zwave_tx_route_cache_set_number_of_repeaters(20, 4);
  zwave_tx_route_cache_set_number_of_repeaters(21, 3);
  zwave_tx_route_cache_set_number_of_repeaters(400, 3);

  TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(0));
  TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(1));
  TEST_ASSERT_EQUAL(2, zwave_tx_route_cache_get_number_of_repeaters(10));
  TEST_ASSERT_EQUAL(5, zwave_tx_route_cache_get_number_of_repeaters(11));
  TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(15));
  TEST_ASSERT_EQUAL(4, zwave_tx_route_cache_get_number_of_repeaters(20));
  TEST_ASSERT_EQUAL(3, zwave_tx_route_cache_get_number_of_repeaters(21));
  TEST_ASSERT_EQUAL(3, zwave_tx_route_cache_get_number_of_repeaters(400));

  zwave_tx_route_cache_init();

  for (zwave_node_id_t i = 0; i < ZW_LR_MAX_NODE_ID; i++) {
    TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(i));
  }
}

void test_zwave_tx_route_cache_maxout_capacity()
{
  for (zwave_node_id_t i = 1; i <= ZWAVE_TX_ROUTE_CACHE_BUFFER_SIZE + 1; i++) {
    zwave_tx_route_cache_set_number_of_repeaters(i, i);
  }
  // Check the last saved element
  TEST_ASSERT_EQUAL(ZWAVE_TX_ROUTE_CACHE_BUFFER_SIZE,
                    zwave_tx_route_cache_get_number_of_repeaters(
                      ZWAVE_TX_ROUTE_CACHE_BUFFER_SIZE));

  // Capacity has been reached after.
  TEST_ASSERT_EQUAL(0,
                    zwave_tx_route_cache_get_number_of_repeaters(
                      ZWAVE_TX_ROUTE_CACHE_BUFFER_SIZE + 1));

  // Clear an entry, e.g. entry 1, by setting 0 repeaters
  zwave_tx_route_cache_set_number_of_repeaters(1, 0);

  // Now we can insert a new one:
  zwave_tx_route_cache_set_number_of_repeaters(
    ZWAVE_TX_ROUTE_CACHE_BUFFER_SIZE + 1,
    ZWAVE_TX_ROUTE_CACHE_BUFFER_SIZE + 1);

  TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(1));
  TEST_ASSERT_EQUAL(ZWAVE_TX_ROUTE_CACHE_BUFFER_SIZE + 1,
                    zwave_tx_route_cache_get_number_of_repeaters(
                      ZWAVE_TX_ROUTE_CACHE_BUFFER_SIZE + 1));
}

void test_zwave_tx_route_cache_link_score_prefers_routers_speed_rssi()
{
  zwapi_tx_report_t direct = {};
  direct.number_of_repeaters = 0;
  direct.ack_rssi            = -50;
  direct.last_route_speed    = ZWAVE_100_KBITS_S;
  direct.beam_250ms          = false;
  direct.beam_1000ms         = false;

  zwapi_tx_report_t routed_fl = {};
  routed_fl.number_of_repeaters = 3;
  routed_fl.ack_rssi            = -90;
  routed_fl.last_route_speed    = ZWAVE_9_6_KBITS_S;
  routed_fl.beam_1000ms         = true;

  zwave_tx_route_cache_update_from_tx_report(4, &direct);
  zwave_tx_route_cache_update_from_tx_report(5, &routed_fl);

  TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(4));
  TEST_ASSERT_EQUAL(3, zwave_tx_route_cache_get_number_of_repeaters(5));

  const uint32_t unknown = zwave_tx_route_cache_link_score(99);
  const uint32_t router  = zwave_tx_route_cache_link_score(4);
  const uint32_t sleepy  = zwave_tx_route_cache_link_score(5);

  TEST_ASSERT_TRUE(router > unknown);
  TEST_ASSERT_TRUE(unknown > sleepy);
}

void test_zwave_tx_route_cache_faster_speed_beats_slower()
{
  zwapi_tx_report_t fast = {};
  fast.number_of_repeaters = 1;
  fast.ack_rssi            = -70;
  fast.last_route_speed    = ZWAVE_100_KBITS_S;

  zwapi_tx_report_t slow = {};
  slow.number_of_repeaters = 1;
  slow.ack_rssi            = -70;
  slow.last_route_speed    = ZWAVE_9_6_KBITS_S;

  zwave_tx_route_cache_update_from_tx_report(8, &fast);
  zwave_tx_route_cache_update_from_tx_report(9, &slow);

  TEST_ASSERT_TRUE(zwave_tx_route_cache_link_score(8)
                   > zwave_tx_route_cache_link_score(9));
}

void test_zwave_tx_route_cache_stronger_rssi_beats_weaker()
{
  zwapi_tx_report_t strong = {};
  strong.number_of_repeaters = 0;
  strong.ack_rssi            = -40;
  strong.last_route_speed    = ZWAVE_100_KBITS_S;

  zwapi_tx_report_t weak = {};
  weak.number_of_repeaters = 0;
  weak.ack_rssi            = -90;
  weak.last_route_speed    = ZWAVE_100_KBITS_S;

  zwave_tx_route_cache_update_from_tx_report(6, &strong);
  zwave_tx_route_cache_update_from_tx_report(7, &weak);

  TEST_ASSERT_TRUE(zwave_tx_route_cache_link_score(6)
                   > zwave_tx_route_cache_link_score(7));
}

void test_zwave_tx_route_cache_direct_zero_hops_keeps_metrics()
{
  zwapi_tx_report_t direct = {};
  direct.number_of_repeaters = 0;
  direct.ack_rssi            = -55;
  direct.last_route_speed    = ZWAVE_100_KBITS_S;

  zwave_tx_route_cache_update_from_tx_report(2, &direct);

  TEST_ASSERT_EQUAL(0, zwave_tx_route_cache_get_number_of_repeaters(2));
  TEST_ASSERT_TRUE(zwave_tx_route_cache_link_score(2)
                   > zwave_tx_route_cache_link_score(99));
}