#include "app.h"

void app_io_test(void);
static keyboard_t key_io_scan(void);
static uint16_t fc135kw_io(fc135kw_io_t io, uint16_t value);
static uint16_t tyt60kw_io(tyt60kw_io_t io, uint16_t value);
static void bidc300100_tx(uint32_t id, uint8_t* data, uint8_t size);
static void lptmrISR(void);

bidc300100_t dcdc;

fc135kw_t fc135kw;
fc135kw_config_t fc135kw_config =
{
    .name = "fc135kw",
    .log = app_log,
    .io = fc135kw_io,
};

tyt60kw_t tyt60kw;
tyt60kw_config_t tyt60kw_config =
{
    .name = "tyt60kw",
    .log = app_log,
    .io = tyt60kw_io,
};

uart_t uart1 =
{
    .instance = INST_LPUART1,
    .state = &lpuart1_State,
    .user_cfg = &lpuart1_InitConfig0,
};
uint8_t uart_tx_buf[1024];
uint8_t uart_rx_buf[1024];

/*--------------------------------------------------------------------------------------------------------*/

void app_init(void)
{
    /** clock */
    CLOCK_DRV_Init(&clockMan1_InitConfig0);

    /** gpio */
    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr);

    /** timer */
    LPTMR_DRV_Init(INST_LPTMR1, &lpTmr1_config0, false);
    INT_SYS_InstallHandler(LPTMR0_IRQn, &lptmrISR, (isr_t*)0);
    INT_SYS_EnableIRQ(LPTMR0_IRQn);
    LPTMR_DRV_StartCounter(INST_LPTMR1);

     /** dma */
    EDMA_DRV_Init(&dmaController1_State,
        &dmaController1_InitConfig0,
        edmaChnStateArray,
        edmaChnConfigArray,
        EDMA_CONFIGURED_CHANNELS_COUNT);

    /** pwm */
    static ftm_state_t ftmStateStruct[4];
    FTM_DRV_Init(INST_FLEXTIMER_PWM0, &flexTimer_pwm0_InitConfig, &ftmStateStruct[0]);
    FTM_DRV_InitPwm(INST_FLEXTIMER_PWM0, &flexTimer_pwm0_PwmConfig);
    FTM_DRV_Init(INST_FLEXTIMER_PWM1, &flexTimer_pwm1_InitConfig, &ftmStateStruct[1]);
    FTM_DRV_InitPwm(INST_FLEXTIMER_PWM1, &flexTimer_pwm1_PwmConfig);
    FTM_DRV_Init(INST_FLEXTIMER_PWM2, &flexTimer_pwm2_InitConfig, &ftmStateStruct[2]);
    FTM_DRV_InitPwm(INST_FLEXTIMER_PWM2, &flexTimer_pwm2_PwmConfig);
    FTM_DRV_Init(INST_FLEXTIMER_PWM3, &flexTimer_pwm3_InitConfig, &ftmStateStruct[3]);
    FTM_DRV_InitPwm(INST_FLEXTIMER_PWM3, &flexTimer_pwm3_PwmConfig);

    /** user */
    key_io_init(key_io_scan);
    app_can_init();

    uart_init(&uart1);
    uart_tx_config(&uart1, &uart_tx_buf[0], sizeof(uart_tx_buf));
    uart_rx_config(&uart1, &uart_rx_buf[0], sizeof(uart_rx_buf));

    bidc300100_init(&dcdc, 0, bidc300100_tx);
    fc135kw_init(&fc135kw, &fc135kw_config);
    tyt60kw_init(&tyt60kw, &tyt60kw_config);
}

void app_run(void)
{
    basic_time();
    cmngr_task();
    menu();
    SYS_RUN_LIGHT(time_flag.bits.t1_00s_clock);

    app_io_test();
}

void app_log(const char* str, uint16_t size)
{
    uart_write(&uart1, (uint8_t*)str, size);
}

void app_io_test(void)
{
    uint32_t port;
    uint8_t buffer[32] = { 0 };
    uint8_t num;
    uint8_t ps;
    bool logic;

    if (time_flag.bits.t0_01s)
    // if (uart_rx_available(&uart1) >= 4)  //TODO 接收讀取會異常，要用時間間隔暫時對策(應該是收到4byte馬上執行導致後面的數據被拉到下一次的第一個位置開始放)
    {
        if (uart_read(&uart1, &buffer[0], 32))
        {
            if (buffer[0] != 'P')
            {
                if ((buffer[0] == 'F') && (buffer[1] == 'C'))
                {
                    if ((buffer[2] == '1') && (buffer[3] == '3') && (buffer[4] == '5'))
                    {
                        fc135kw_remove_bubble(&fc135kw, (buffer[5] == 'R'));
                    }
                }
                else
                {
                    switch (buffer[0])
                    {
                        case 'A': { port = (uint32_t)PTA; break; }
                        case 'B': { port = (uint32_t)PTB; break; }
                        case 'C': { port = (uint32_t)PTC; break; }
                        case 'D': { port = (uint32_t)PTD; break; }
                        case 'E': { port = (uint32_t)PTE; break; }
                        default: { return; }
                    }

                    num = (buffer[1] - '0') * 10;
                    num += buffer[2] - '0';

                    if (num < 18)
                    {
                        if (buffer[3] == 'R')
                        {
                            logic = ((PINS_DRV_ReadPins((GPIO_Type* const)port) >> (num)) & 0x01);

                            buffer[3] = '=';
                            buffer[4] = (logic ? '1' : '0');
                            buffer[5] = '\r';
                            buffer[6] = '\n';
                            uart_write(&uart1, &buffer[0], 7);
                        }
                        else if (buffer[3] == 'W')
                        {
                            logic = ((buffer[3] - '0') == 1);
                            PINS_DRV_WritePin((GPIO_Type* const)port, (1 << num), logic);
                        }
                        else
                        {
                            PINS_DRV_TogglePins((GPIO_Type* const)port, (1 << num));
                        }
                    }
                }
            }
            else if ((buffer[1] == 'W') && (buffer[2] == 'M') && (buffer[5] == '='))
            {
                num = buffer[4] - '0';

                if (num > 2)
                {
                    return;
                }

                ps = (buffer[6] - '0') * 100;
                ps += (buffer[7] - '0') * 10;
                ps += (buffer[8] - '0');

                if (ps > 100)
                {
                    return;
                }

                switch (buffer[3])
                {
                    case '0':
                    {
                        pwm_duty(INST_FLEXTIMER_PWM0, num, ps);
                        break;
                    }
                    case '1':
                    {
                        if (num > 1) { return; }
                        pwm_duty(INST_FLEXTIMER_PWM1, num, ps);
                        break;
                    }
                    case '2':
                    {
                        pwm_duty(INST_FLEXTIMER_PWM2, num, ps);
                        break;
                    }
                    case '3':
                    {
                        pwm_duty(INST_FLEXTIMER_PWM3, num, ps);
                        break;
                    }
                    default: break;
                }
            }

        }
    }
}

/*--------------------------------------------------------------------------------------------------------*/

static keyboard_t key_io_scan(void)
{
    keyboard_t keyboard = { 0 };

    if (BUTTON_ON() || KEY_1()) { keyboard.bits.key_1 = 1; }
    if (BUTTON_OFF() || KEY_2()) { keyboard.bits.key_2 = 1; }

    return keyboard;
}

static void bidc300100_tx(uint32_t id, uint8_t* data, uint8_t size)
{
    can_message_t message =
    {
        .id = id,
        .idt = 1,
        .dlc = size,
        .data.bytes[0] = data[0],
        .data.bytes[1] = data[1],
        .data.bytes[2] = data[2],
        .data.bytes[3] = data[3],
        .data.bytes[4] = data[4],
        .data.bytes[5] = data[5],
        .data.bytes[6] = data[6],
        .data.bytes[7] = data[7],
    };

    cmngr_tx_message(&can2_manager, &message);
}

static uint16_t fc135kw_io(fc135kw_io_t io, uint16_t value)
{
    switch (io)
    {
        case FC135KW_IO_24V_POWER: { FC_135KW_POWER((value != 0)); break; }
        case FC135KW_IO_IGN: { FC_135KW_IGN((value != 0)); break; }
        default: { break; }
    }

    return 0;
}

static uint16_t tyt60kw_io(tyt60kw_io_t io, uint16_t value)
{
    switch (io)
    {
        case TYT60KW_IO_POWER: { FC_TYT60KW_POWER((value != 0)); break; }
        case TYT60KW_IO_IGN: { FC_TYT60KW_IGN((value != 0)); break; }
        case TYT60KW_IO_FAN_H: { FC_TYT60KW_FAN_H(value); break; }
        case TYT60KW_IO_FAN_L: { FC_TYT60KW_FAN_L(value); break; }
        default: { break; }
    }

    return 0;
}

static void lptmrISR(void)
{
    //1ms
    LPTMR_DRV_ClearCompareFlag(INST_LPTMR1);
    basic_time_from_10ms_irp();
}

void pwm_duty(uint8_t num, uint8_t ch, uint16_t duty)
{
    uint8_t id;

    switch (num)
    {
        case INST_FLEXTIMER_PWM0:
        {
            id = flexTimer_pwm0_IndependentChannelsConfig[ch].hwChannelId;
            break;
        }
        case INST_FLEXTIMER_PWM1:
        {
            id = flexTimer_pwm1_IndependentChannelsConfig[ch].hwChannelId;
            break;
        }
        case INST_FLEXTIMER_PWM2:
        {
            id = flexTimer_pwm2_IndependentChannelsConfig[ch].hwChannelId;
            break;
        }
        case INST_FLEXTIMER_PWM3:
        {
            id = flexTimer_pwm3_IndependentChannelsConfig[ch].hwChannelId;
            break;
        }
        default: { return; }
    }

    FTM_DRV_UpdatePwmChannel(num,
        id,
        FTM_PWM_UPDATE_IN_TICKS,
        (uint16_t)(((uint32_t)duty * 24000) / 100),
        0U,
        true);
}
