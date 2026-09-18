/*
 * fault_handlers.c
 *
 * Dedicated fault exception handlers for Cortex-M23 (RA2E1).
 * Captures stacked register frame (PC, LR, R0-R3, R12, xPSR)
 * and SCB fault registers into g_fault_diagnostic for easy
 * inspection in e2 studio debugger.
 */

#include "hal_data.h"
#include <stdint.h>

typedef struct
{
    uint32_t ipsr;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    uint32_t shcsr;
    uint32_t icsr;
} fault_diagnostic_t;

volatile fault_diagnostic_t g_fault_diagnostic;

void fault_diagnostic_capture_c(uint32_t *stack_ptr, uint32_t ipsr);
void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName);

static void fault_uart_putc(char c)
{
    if (g_uart0_ctrl.p_reg != NULL)
    {
        while (0u == g_uart0_ctrl.p_reg->SSR_b.TDRE)
        {
        }
        g_uart0_ctrl.p_reg->TDR = (uint8_t) c;
    }
}

static void fault_uart_puts(const char *str)
{
    while (*str != '\0')
    {
        fault_uart_putc(*str++);
    }
}

static void fault_uart_puthex(uint32_t val)
{
    const char hex_chars[] = "0123456789ABCDEF";
    for (int32_t i = 28; i >= 0; i -= 4)
    {
        fault_uart_putc(hex_chars[(val >> (uint32_t) i) & 0x0Fu]);
    }
}

extern void * pxCurrentTCB;

void fault_diagnostic_capture_c(uint32_t *stack_ptr, uint32_t ipsr)
{
    g_fault_diagnostic.ipsr = ipsr;
    if (stack_ptr != NULL)
    {
        g_fault_diagnostic.r0   = stack_ptr[0];
        g_fault_diagnostic.r1   = stack_ptr[1];
        g_fault_diagnostic.r2   = stack_ptr[2];
        g_fault_diagnostic.r3   = stack_ptr[3];
        g_fault_diagnostic.r12  = stack_ptr[4];
        g_fault_diagnostic.lr   = stack_ptr[5];
        g_fault_diagnostic.pc   = stack_ptr[6];
        g_fault_diagnostic.xpsr = stack_ptr[7];
    }
    g_fault_diagnostic.shcsr = SCB->SHCSR;
    g_fault_diagnostic.icsr  = SCB->ICSR;

    fault_uart_puts("\r\n[CRITICAL FAULT] IPSR=0x");
    fault_uart_puthex(g_fault_diagnostic.ipsr);
    fault_uart_puts(" PC=0x");
    fault_uart_puthex(g_fault_diagnostic.pc);
    fault_uart_puts(" LR=0x");
    fault_uart_puthex(g_fault_diagnostic.lr);
    fault_uart_puts(" SP=0x");
    fault_uart_puthex((uint32_t) stack_ptr);
    fault_uart_puts("\r\n  R0=0x");
    fault_uart_puthex(g_fault_diagnostic.r0);
    fault_uart_puts(" R1=0x");
    fault_uart_puthex(g_fault_diagnostic.r1);
    fault_uart_puts(" R2=0x");
    fault_uart_puthex(g_fault_diagnostic.r2);
    fault_uart_puts(" R3=0x");
    fault_uart_puthex(g_fault_diagnostic.r3);
    fault_uart_puts(" R12=0x");
    fault_uart_puthex(g_fault_diagnostic.r12);
    fault_uart_puts(" xPSR=0x");
    fault_uart_puthex(g_fault_diagnostic.xpsr);
    fault_uart_puts("\r\n  SHCSR=0x");
    fault_uart_puthex(g_fault_diagnostic.shcsr);
    fault_uart_puts(" ICSR=0x");
    fault_uart_puthex(g_fault_diagnostic.icsr);
    fault_uart_puts(" TCB=0x");
    fault_uart_puthex((uint32_t) pxCurrentTCB);
    fault_uart_puts("\r\n");

    if (stack_ptr != NULL)
    {
        fault_uart_puts("  Stack words: ");
        for (uint32_t i = 0; i < 8; i++)
        {
            fault_uart_puthex(stack_ptr[i]);
            fault_uart_putc(' ');
        }
        fault_uart_puts("\r\n  Stack words[8..15]: ");
        for (uint32_t i = 8; i < 16; i++)
        {
            fault_uart_puthex(stack_ptr[i]);
            fault_uart_putc(' ');
        }
        fault_uart_puts("\r\n");
    }

    __BKPT(0);
    while (1)
    {
        /* Trapped in fault handler */
    }
}

void HardFault_Handler(void) __attribute__((naked));
void HardFault_Handler(void)
{
    __asm volatile(
        "mrs  r1, ipsr                 \n"
        "movs r0, #4                   \n"
        "mov  r2, lr                   \n"
        "tst  r0, r2                   \n"
        "beq  1f                       \n"
        "mrs  r0, psp                  \n"
        "b    2f                       \n"
        "1:                            \n"
        "mrs  r0, msp                  \n"
        "2:                            \n"
        "ldr  r3, =fault_diagnostic_capture_c \n"
        "bx   r3                       \n"
    );
}

void MemManage_Handler(void) __attribute__((alias("HardFault_Handler")));
void BusFault_Handler(void) __attribute__((alias("HardFault_Handler")));
void UsageFault_Handler(void) __attribute__((alias("HardFault_Handler")));
void SecureFault_Handler(void) __attribute__((alias("HardFault_Handler")));

void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName)
{
    FSP_PARAMETER_NOT_USED(xTask);
    fault_uart_puts("\r\n[STACK OVERFLOW] In Task: ");
    if (pcTaskName != NULL)
    {
        fault_uart_puts(pcTaskName);
    }
    fault_uart_puts("\r\n");

    __BKPT(0);
    while (1)
    {
    }
}


