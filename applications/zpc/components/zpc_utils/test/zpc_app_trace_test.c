/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *****************************************************************************/
#include "unity.h"
#include "zpc_app_trace.h"

void setUp(void)
{
  zpc_app_trace_init();
}

void tearDown(void) {}

void test_zpc_app_trace_follows_attr_and_session(void)
{
  const zpc_trace_id_t id = zpc_app_trace_begin("resolver.SET", 12, 1001);
  TEST_ASSERT_NOT_EQUAL(0, id);
  TEST_ASSERT_EQUAL(id, zpc_app_trace_for_attr(1001));

  const void *session = (const void *)(uintptr_t)0xabc;
  zpc_app_trace_bind_session(session, id);
  TEST_ASSERT_EQUAL(id, zpc_app_trace_for_session(session));

  zpc_app_trace_event(id, "tx.enqueue", SL_STATUS_OK, "queued");
  zpc_app_trace_end(id, "ok");

  TEST_ASSERT_EQUAL(0, zpc_app_trace_for_attr(1001));
  TEST_ASSERT_EQUAL(0, zpc_app_trace_for_session(session));
}

void test_zpc_app_trace_reuse_attr_after_defer_keeps_same_id(void)
{
  const zpc_trace_id_t first = zpc_app_trace_begin("resolver.SET", 5, 77);
  zpc_app_trace_event(first, "resolver.defer", SL_STATUS_NOT_READY, "full");
  TEST_ASSERT_EQUAL(first, zpc_app_trace_for_attr(77));
}

void test_zpc_app_trace_current_joins_nested_enqueue(void)
{
  const zpc_trace_id_t id = zpc_app_trace_begin("resolver.SET", 9, 3);
  zpc_app_trace_set_current(id);
  TEST_ASSERT_EQUAL(id, zpc_app_trace_current());
  zpc_app_trace_set_current(0);
  TEST_ASSERT_EQUAL(0, zpc_app_trace_current());
  zpc_app_trace_end(id, "ok");
}

#ifdef ZPC_APP_TRACE_STANDALONE
int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_zpc_app_trace_follows_attr_and_session);
  RUN_TEST(test_zpc_app_trace_reuse_attr_after_defer_keeps_same_id);
  RUN_TEST(test_zpc_app_trace_current_joins_nested_enqueue);
  return UNITY_END();
}
#endif
