/*
 * app_log.c
 *
 * Thread-safe application logging over SCI0 (g_uart0).
 *
 * SCI0 is dedicated to debug logging:
 *
 *     RA2E1 P101 (TXD0) -> FT232RL RX
 *     RA2E1 P104 (RXD0) -> FT232RL TX
 *
 * The logger can be called from multiple FreeRTOS tasks.
 *
 * Important:
 * R_SCI_UART_Write() is asynchronous. Therefore a mutex alone is
 * not sufficient. We serialize access with a mutex AND wait until
 * the current transmission has completed before allowing another
 * task to write.
 */

#include "app/app_log.h"
#include "hal_data.h"

#include "common_data.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define APP_LOG_BUFFER_SIZE       (256u)
#define APP_LOG_TX_TIMEOUT_MS     (100u)

/*
 * Logger state.
 */
static bool s_initialized = false;
static bool s_enabled = (APP_LOG_DEFAULT_ENABLED != 0);
static bool s_tuning_active = (APP_LOG_TUNING_MODE != 0);

/*
 * Protects the complete UART transaction.
 *
 * Only one FreeRTOS task can use SCI0 for logging at a time.
 */
static SemaphoreHandle_t s_log_mutex = NULL;

static StaticSemaphore_t s_log_mutex_buffer;


/*
 * Convert log level to text.
 */
static const char * app_log_level_string(app_log_level_t level)
{
    switch (level)
    {
        case APP_LOG_ERROR:
            return "ERROR";

        case APP_LOG_WARNING:
            return "WARNING";

        case APP_LOG_INFO:
            return "INFO";

        case APP_LOG_DEBUG:
            return "DEBUG";

        default:
            return "UNKNOWN";
    }
}


/*
 * Initialize SCI0 debug UART and logger synchronization.
 */
void app_log_init(void)
{
    if (s_initialized)
    {
        return;
    }

    /*
     * Do not initialize the debug logger while QE tuning owns the
     * serial interface.
     */
    if (s_tuning_active)
    {
        return;
    }

    /*
     * Create the mutex before enabling logging.
     */
    s_log_mutex = xSemaphoreCreateMutexStatic(&s_log_mutex_buffer);

    if (NULL == s_log_mutex)
    {
        return;
    }

    /*
     * SCI0 / g_uart0 is the dedicated debug UART (P101 TX / P104 RX -> FT232 COM8).
     * We pass p_callback = NULL so logging never triggers Tuya's hal_uart_callback.
     */
    uart_cfg_t log_uart_cfg = g_uart0_cfg;
    log_uart_cfg.p_callback = NULL;
    log_uart_cfg.p_context  = NULL;

    fsp_err_t err = R_SCI_UART_Open(
        &g_uart0_ctrl,
        &log_uart_cfg);

    if (FSP_SUCCESS != err)
    {
        vSemaphoreDelete(s_log_mutex);
        s_log_mutex = NULL;

        return;
    }

    s_initialized = true;
}


/*
 * Enable / disable application logging.
 */
void app_log_set_enabled(bool enabled)
{
    s_enabled = enabled;
}


/*
 * Enable / disable tuning mode.
 *
 * When tuning mode is active, normal application logging is disabled
 * because QE may use its own UART communication.
 */
void app_log_set_tuning_active(bool active)
{
    s_tuning_active = active;

    if (active)
    {
        s_enabled = false;
    }
}


/*
 * Return whether logging is currently available.
 */
bool app_log_is_enabled(void)
{
    return s_initialized &&
           s_enabled &&
           !s_tuning_active &&
           (NULL != s_log_mutex);
}


static char s_log_message_buffer[APP_LOG_BUFFER_SIZE];

/*
 * Write one complete log message.
 *
 * This function is safe to call from multiple FreeRTOS tasks.
 */
void app_log_write(
    app_log_level_t level,
    const char * format,
    ...)
{
    if ((NULL == format) ||
        !app_log_is_enabled())
    {
        return;
    }

    /*
     * Serialize access to SCI0 and s_log_message_buffer.
     *
     * Taking the mutex first avoids allocating 256 bytes on the task stack.
     */
    if (pdTRUE != xSemaphoreTake(
            s_log_mutex,
            pdMS_TO_TICKS(APP_LOG_TX_TIMEOUT_MS)))
    {
        return;
    }

    const char * level_string =
        app_log_level_string(level);

    /*
     * Add log prefix.
     *
     * Example:
     *
     *     [INFO] Touch thread started
     */
    int prefix_length = snprintf(
        s_log_message_buffer,
        sizeof(s_log_message_buffer),
        "[%s] ",
        level_string);

    if ((prefix_length < 0) ||
        ((size_t) prefix_length >= sizeof(s_log_message_buffer)))
    {
        xSemaphoreGive(s_log_mutex);
        return;
    }

    /*
     * Format user message.
     */
    va_list args;

    va_start(args, format);

    int body_length = vsnprintf(
        &s_log_message_buffer[prefix_length],
        sizeof(s_log_message_buffer) - (size_t) prefix_length,
        format,
        args);

    va_end(args);

    if (body_length < 0)
    {
        xSemaphoreGive(s_log_mutex);
        return;
    }

    size_t total_length =
        (size_t) prefix_length +
        (size_t) body_length;

    /*
     * Reserve space for CR/LF and terminating NUL.
     */
    if (total_length >
        (sizeof(s_log_message_buffer) - 3u))
    {
        total_length = sizeof(s_log_message_buffer) - 3u;
    }

    /*
     * Add CR/LF.
     */
    s_log_message_buffer[total_length++] = '\r';
    s_log_message_buffer[total_length++] = '\n';

    /*
     * Keep the buffer NUL terminated even though the UART write
     * does not require it.
     */
    s_log_message_buffer[total_length] = '\0';

    /*
     * Transmit over SCI0.
     */
    fsp_err_t err = R_SCI_UART_Write(
        &g_uart0_ctrl,
        (uint8_t *) s_log_message_buffer,
        (uint32_t) total_length);

    if (FSP_SUCCESS == err)
    {
        /*
         * R_SCI_UART_Write() is asynchronous.
         *
         * Wait until the SCI hardware reports that transmission
         * has completely finished.
         *
         * TEND = Transmit End.
         */
        TickType_t start_tick = xTaskGetTickCount();

        while (0u == g_uart0_ctrl.p_reg->SSR_b.TEND)
        {
            if ((xTaskGetTickCount() - start_tick) >=
                pdMS_TO_TICKS(APP_LOG_TX_TIMEOUT_MS))
            {
                break;
            }

            taskYIELD();
        }
    }

    /*
     * Release SCI0 for the next logging task.
     */
    xSemaphoreGive(s_log_mutex);
}