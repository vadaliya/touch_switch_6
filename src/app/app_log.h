/*
 * app_log.h
 *
 * Runtime diagnostics over the QE UART port. Logging must be disabled while
 * QE Touch tuning is active because both functions share the serial port.
 */
#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    APP_LOG_ERROR = 0,
    APP_LOG_WARNING,
    APP_LOG_INFO,
    APP_LOG_DEBUG
} app_log_level_t;

/* Set to 1 while running the QE Touch tuning tool. The logger and QE tuning
 * share the same physical P109/P110 serial port and cannot run together. */
#ifndef APP_LOG_TUNING_MODE
#define APP_LOG_TUNING_MODE (0)
#endif

#ifndef APP_LOG_DEFAULT_ENABLED
#define APP_LOG_DEFAULT_ENABLED (1)
#endif

void app_log_init(void);
void app_log_set_enabled(bool enabled);
void app_log_set_tuning_active(bool active);
bool app_log_is_enabled(void);
void app_log_write(app_log_level_t level, const char *format, ...);

#define LOG_ERROR(...)   app_log_write(APP_LOG_ERROR, __VA_ARGS__)
#define LOG_WARNING(...) app_log_write(APP_LOG_WARNING, __VA_ARGS__)
#define LOG_INFO(...)    app_log_write(APP_LOG_INFO, __VA_ARGS__)
#define LOG_DEBUG(...)   app_log_write(APP_LOG_DEBUG, __VA_ARGS__)

#endif
