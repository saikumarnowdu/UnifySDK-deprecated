/******************************************************************************
 * Minimal logging stub for the Z-Wave hardware test harness (no Boost dependency).
 */
#include "sl_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static sl_log_level_t global_level = SL_LOG_WARNING;

void sl_log_set_level(sl_log_level_t level)
{
  global_level = level;
}

sl_log_level_t sl_log_get_level(void)
{
  return global_level;
}

void sl_log_set_tag_level(const char *tag, sl_log_level_t level)
{
  (void)tag;
  global_level = level;
}

void sl_log_unset_tag_level(const char *tag)
{
  (void)tag;
}

sl_status_t sl_log_level_from_string(const char *level, sl_log_level_t *result)
{
  if (level == NULL || result == NULL) {
    return SL_STATUS_FAIL;
  }
  if (strcmp(level, "debug") == 0 || strcmp(level, "d") == 0) {
    *result = SL_LOG_DEBUG;
  } else if (strcmp(level, "info") == 0 || strcmp(level, "i") == 0) {
    *result = SL_LOG_INFO;
  } else if (strcmp(level, "warning") == 0 || strcmp(level, "w") == 0) {
    *result = SL_LOG_WARNING;
  } else if (strcmp(level, "error") == 0 || strcmp(level, "e") == 0) {
    *result = SL_LOG_ERROR;
  } else if (strcmp(level, "critical") == 0 || strcmp(level, "c") == 0) {
    *result = SL_LOG_CRITICAL;
  } else {
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

void sl_log_read_config(void)
{
}

void sl_log(const char *const tag, sl_log_level_t level, const char *fmtstr, ...)
{
  if (level < global_level) {
    return;
  }
  const char *lvl = "info";
  switch (level) {
    case SL_LOG_DEBUG:
      lvl = "debug";
      break;
    case SL_LOG_INFO:
      lvl = "info";
      break;
    case SL_LOG_WARNING:
      lvl = "warning";
      break;
    case SL_LOG_ERROR:
      lvl = "error";
      break;
    case SL_LOG_CRITICAL:
      lvl = "critical";
      break;
    default:
      break;
  }
  fprintf(stderr, "[%s][%s] ", tag != NULL ? tag : "log", lvl);
  va_list args;
  va_start(args, fmtstr);
  vfprintf(stderr, fmtstr, args);
  va_end(args);
}
