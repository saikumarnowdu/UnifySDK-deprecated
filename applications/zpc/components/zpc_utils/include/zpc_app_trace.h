/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *****************************************************************************/
#ifndef ZPC_APP_TRACE_H
#define ZPC_APP_TRACE_H

#include <stdint.h>
#include <stddef.h>
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application trace ID. 0 means "no trace".
 *
 * One ID follows a single resolver/TX request across:
 * resolver.attempt → tx.enqueue → tx.on_air → tx.complete → resolver.complete
 *
 * Log tag: `zpc_trace`. Filter Unify logs with that tag.
 */
typedef uint64_t zpc_trace_id_t;

void zpc_app_trace_init(void);

zpc_trace_id_t zpc_app_trace_begin(const char *operation,
                                   uint16_t node_id,
                                   uintptr_t attr_key);

void zpc_app_trace_event(zpc_trace_id_t id,
                         const char *span,
                         sl_status_t status,
                         const char *detail);

void zpc_app_trace_end(zpc_trace_id_t id, const char *result);

void zpc_app_trace_bind_attr(uintptr_t attr_key, zpc_trace_id_t id);
zpc_trace_id_t zpc_app_trace_for_attr(uintptr_t attr_key);

void zpc_app_trace_bind_session(const void *session, zpc_trace_id_t id);
zpc_trace_id_t zpc_app_trace_for_session(const void *session);

void zpc_app_trace_unbind_attr(uintptr_t attr_key);
void zpc_app_trace_unbind_session(const void *session);

/** Single-thread "current request" so nested TX enqueue joins this trace. */
void zpc_app_trace_set_current(zpc_trace_id_t id);
zpc_trace_id_t zpc_app_trace_current(void);

/** End traces that are not owned by the resolver (no attr key). */
void zpc_app_trace_end_if_no_attr(zpc_trace_id_t id, const char *result);

#ifdef __cplusplus
}
#endif

#endif
