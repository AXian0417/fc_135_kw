#ifndef _app_h_
#define _app_h_

#include "main.h"
#include <stdint.h>
#include "define_io.h"
#include "basic_time.h"
#include "can_manager.h"
#include "keyboard.h"
#include "s32_can.h"
#include "s32_uart.h"
#include "menu.h"

#include "fc135kw.h"
#include "toyota60kw.h"
#include "bidc300100.h"

#define TYT60KW_HOST_CAN_TX_ID                  (0x00BB0000 | (((uint32_t)0x81) << 8) | 0x10)
#define TYT60KW_HOST_CAN_RX_ID                  (0x00AA0000 | (((uint32_t)0x81) << 8) | 0x10)
#define FC135KW_HOST_CAN_TX_ID                  (0x00BB0000 | (((uint32_t)0x82) << 8) | 0x10)
#define FC135KW_HOST_CAN_RX_ID                  (0x00AA0000 | (((uint32_t)0x82) << 8) | 0x10)

#define app_logger(fmt, ...)                    uart_printf(&uart1, fmt, ## __VA_ARGS__)

extern uart_t uart1;
extern cmngr_t can0_manager, can1_manager, can2_manager;
extern fc135kw_t fc135kw;
extern tyt60kw_t tyt60kw;
extern bidc300100_t dcdc;

void app_init(void);
void app_run(void);
void app_log(const char* str, uint16_t size);
void app_can_init(void);
void pwm_duty(uint8_t num, uint8_t ch, uint16_t duty);

#endif
