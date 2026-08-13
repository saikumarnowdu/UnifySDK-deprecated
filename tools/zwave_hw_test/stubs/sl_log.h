#ifndef ZWAVE_HW_TEST_SL_LOG_H
#define ZWAVE_HW_TEST_SL_LOG_H

#include "sl_status.h"
#include <stdio.h>

typedef enum sl_log_level {
  SL_LOG_DEBUG,
  SL_LOG_INFO,
  SL_LOG_WARNING,
  SL_LOG_ERROR,
  SL_LOG_CRITICAL
} sl_log_level_t;

void sl_log_set_level(sl_log_level_t level);
sl_log_level_t sl_log_get_level(void);
void sl_log_set_tag_level(const char *tag, sl_log_level_t level);
void sl_log_unset_tag_level(const char *tag);
sl_status_t sl_log_level_from_string(const char *level, sl_log_level_t *result);
void sl_log_read_config(void);
void sl_log(const char *const tag, sl_log_level_t level, const char *fmtstr, ...);

#define sl_log_debug(tag, fmtstr, ...) \
  sl_log(tag, SL_LOG_DEBUG, fmtstr, ##__VA_ARGS__)
#define sl_log_info(tag, fmtstr, ...) \
  sl_log(tag, SL_LOG_INFO, fmtstr, ##__VA_ARGS__)
#define sl_log_warning(tag, fmtstr, ...) \
  sl_log(tag, SL_LOG_WARNING, fmtstr, ##__VA_ARGS__)
#define sl_log_error(tag, fmtstr, ...) \
  sl_log(tag, SL_LOG_ERROR, fmtstr, ##__VA_ARGS__)
#define sl_log_critical(tag, fmtstr, ...) \
  sl_log(tag, SL_LOG_CRITICAL, fmtstr, ##__VA_ARGS__)

#endif
