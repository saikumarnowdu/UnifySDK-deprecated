/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *****************************************************************************/
#include "zpc_app_trace.h"
#include "sl_log.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define LOG_TAG "zpc_trace"

#define ZPC_APP_TRACE_SLOTS 128

typedef struct {
  zpc_trace_id_t id;
  char operation[32];
  uint16_t node_id;
  uintptr_t attr_key;
  const void *session;
  uint64_t start_ns;
  bool in_use;
} zpc_app_trace_slot_t;

static zpc_app_trace_slot_t slots[ZPC_APP_TRACE_SLOTS];
static zpc_trace_id_t next_id       = 1;
static zpc_trace_id_t current_trace = 0;

static uint64_t now_ns(void)
{
  struct timespec ts = {0};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static zpc_app_trace_slot_t *find_id(zpc_trace_id_t id)
{
  if (id == 0) {
    return NULL;
  }
  for (size_t i = 0; i < ZPC_APP_TRACE_SLOTS; ++i) {
    if (slots[i].in_use && slots[i].id == id) {
      return &slots[i];
    }
  }
  return NULL;
}

static zpc_app_trace_slot_t *alloc_slot(void)
{
  for (size_t i = 0; i < ZPC_APP_TRACE_SLOTS; ++i) {
    if (!slots[i].in_use) {
      memset(&slots[i], 0, sizeof(slots[i]));
      slots[i].in_use = true;
      return &slots[i];
    }
  }
  return NULL;
}

void zpc_app_trace_init(void)
{
  memset(slots, 0, sizeof(slots));
  next_id       = 1;
  current_trace = 0;
}

zpc_trace_id_t zpc_app_trace_begin(const char *operation,
                                   uint16_t node_id,
                                   uintptr_t attr_key)
{
  zpc_app_trace_slot_t *slot = alloc_slot();
  if (slot == NULL) {
    sl_log_debug(LOG_TAG, "trace table full, dropping begin");
    return 0;
  }
  slot->id       = next_id++;
  slot->node_id  = node_id;
  slot->attr_key = attr_key;
  slot->start_ns = now_ns();
  if (operation != NULL) {
    snprintf(slot->operation, sizeof(slot->operation), "%s", operation);
  }
  sl_log_info(LOG_TAG,
              "{\"trace\":\"%016llx\",\"span\":\"begin\",\"op\":\"%s\","
              "\"node\":%u,\"attr\":%lu}",
              (unsigned long long)slot->id,
              slot->operation,
              (unsigned)node_id,
              (unsigned long)attr_key);
  return slot->id;
}

void zpc_app_trace_event(zpc_trace_id_t id,
                         const char *span,
                         sl_status_t status,
                         const char *detail)
{
  const zpc_app_trace_slot_t *slot = find_id(id);
  if (slot == NULL) {
    return;
  }
  sl_log_info(LOG_TAG,
              "{\"trace\":\"%016llx\",\"span\":\"%s\",\"op\":\"%s\","
              "\"node\":%u,\"attr\":%lu,\"session\":\"%p\",\"status\":0x%lx,"
              "\"detail\":\"%s\"}",
              (unsigned long long)slot->id,
              span ? span : "",
              slot->operation,
              (unsigned)slot->node_id,
              (unsigned long)slot->attr_key,
              slot->session,
              (unsigned long)status,
              detail ? detail : "");
}

void zpc_app_trace_end(zpc_trace_id_t id, const char *result)
{
  zpc_app_trace_slot_t *slot = find_id(id);
  if (slot == NULL) {
    return;
  }
  const uint64_t elapsed_us = (now_ns() - slot->start_ns) / 1000ull;
  sl_log_info(LOG_TAG,
              "{\"trace\":\"%016llx\",\"span\":\"end\",\"op\":\"%s\","
              "\"node\":%u,\"attr\":%lu,\"result\":\"%s\",\"elapsed_us\":%llu}",
              (unsigned long long)slot->id,
              slot->operation,
              (unsigned)slot->node_id,
              (unsigned long)slot->attr_key,
              result ? result : "",
              (unsigned long long)elapsed_us);
  slot->in_use = false;
}

void zpc_app_trace_bind_attr(uintptr_t attr_key, zpc_trace_id_t id)
{
  zpc_app_trace_slot_t *slot = find_id(id);
  if (slot == NULL) {
    return;
  }
  slot->attr_key = attr_key;
}

zpc_trace_id_t zpc_app_trace_for_attr(uintptr_t attr_key)
{
  if (attr_key == 0) {
    return 0;
  }
  for (size_t i = 0; i < ZPC_APP_TRACE_SLOTS; ++i) {
    if (slots[i].in_use && slots[i].attr_key == attr_key) {
      return slots[i].id;
    }
  }
  return 0;
}

void zpc_app_trace_bind_session(const void *session, zpc_trace_id_t id)
{
  zpc_app_trace_slot_t *slot = find_id(id);
  if (slot == NULL) {
    return;
  }
  slot->session = session;
}

zpc_trace_id_t zpc_app_trace_for_session(const void *session)
{
  if (session == NULL) {
    return 0;
  }
  for (size_t i = 0; i < ZPC_APP_TRACE_SLOTS; ++i) {
    if (slots[i].in_use && slots[i].session == session) {
      return slots[i].id;
    }
  }
  return 0;
}

void zpc_app_trace_unbind_attr(uintptr_t attr_key)
{
  for (size_t i = 0; i < ZPC_APP_TRACE_SLOTS; ++i) {
    if (slots[i].in_use && slots[i].attr_key == attr_key) {
      slots[i].attr_key = 0;
    }
  }
}

void zpc_app_trace_unbind_session(const void *session)
{
  for (size_t i = 0; i < ZPC_APP_TRACE_SLOTS; ++i) {
    if (slots[i].in_use && slots[i].session == session) {
      slots[i].session = NULL;
    }
  }
}

void zpc_app_trace_set_current(zpc_trace_id_t id)
{
  current_trace = id;
}

zpc_trace_id_t zpc_app_trace_current(void)
{
  return current_trace;
}

void zpc_app_trace_end_if_no_attr(zpc_trace_id_t id, const char *result)
{
  const zpc_app_trace_slot_t *slot = find_id(id);
  if (slot == NULL || slot->attr_key != 0) {
    return;
  }
  zpc_app_trace_end(id, result);
}
