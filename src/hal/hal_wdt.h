/*
 * hal_wdt.h
 *
 * Thin wrapper over FSP's r_wdt driver (g_wdt0). Used by system_thread_entry.c,
 * which only refreshes the watchdog after confirming every other critical
 * thread checked in (see HEARTBEAT_ALL_BITS in app_common.h) -- that's the
 * actual "only refresh if everything's healthy" logic; this file is just
 * the two register-level operations underneath it.
 *
 * DESIGN CHOICE -- Register-start vs. Auto-start mode:
 * RA's WDT supports two modes:
 *   - Register-start: software calls R_WDT_Open() to begin counting;
 *     timeout period is set in the module's Properties in the
 *     Configurator, freely reconfigurable during development.
 *   - Auto-start: WDT begins counting automatically from reset, before
 *     any of your code runs, configured via Option Setting Memory
 *     (OFS1) in the BSP Properties rather than the WDT module itself.
 *     Some OFS1 bits are effectively permanent once programmed on
 *     certain devices -- a real hazard during active development
 *     (e.g. it will reset you mid-breakpoint too).
 *
 * This wrapper assumes REGISTER-START mode: hal_wdt_init() calls
 * R_WDT_Open(), which CONFIGURES the WDT but does NOT start the
 * countdown -- per Renesas's own documented WDT flow, the countdown
 * only actually begins on the FIRST call to R_WDT_Refresh(). That
 * first refresh happens naturally the first time system_thread_entry.c
 * completes a healthy check (~2s after boot, per its check period),
 * so there's a brief window right after hal_wdt_init() where the WDT
 * is configured but not yet counting -- acceptable here since nothing
 * time-critical runs in that window, but worth knowing if you ever
 * need the WDT counting from the literal first instruction.
 *
 * This satisfies the spec's "always-enabled" requirement in practice
 * (started via the first refresh, never stopped, 20s timeout) without
 * auto-start mode's debugging hazards (auto-start begins counting from
 * reset itself, before any of your code runs, and can make breakpoint
 * debugging fight the watchdog). If your project specifically needs
 * true hardware auto-start-from-reset (e.g. for a safety certification
 * requirement), that's a BSP Option Setting Memory (OFS1) change, not
 * something this file can provide -- flag it if you need that discussion.
 */

#ifndef HAL_WDT_H
#define HAL_WDT_H

#include "hal_data.h"   /* FSP-generated: g_wdt0 instance */
#include <stdbool.h>

/**
 * hal_wdt_init
 * Opens/configures g_wdt0. Per Renesas's documented WDT flow, this
 * does NOT start the countdown -- the first call to hal_wdt_refresh()
 * does that (see header comment). Call once at startup, before the
 * scheduler starts any thread.
 */
void hal_wdt_init(void);

/**
 * hal_wdt_refresh
 * Feeds the watchdog, resetting its countdown (and starting it, if
 * this is the first call since hal_wdt_init()). Call ONLY when you've
 * actually confirmed system health -- see system_thread_entry.c's
 * heartbeat-bit gating logic, which is where that decision is made.
 * This function itself does not make that judgment; it just feeds.
 */
void hal_wdt_refresh(void);

#endif /* HAL_WDT_H */