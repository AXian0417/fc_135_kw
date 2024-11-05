#include "app.h"

static cmngr_err_t init(const void* handle);
static cmngr_err_t deinit(const void* handle);
static void filter(const void* handle);
static cmngr_err_t tx(const void* handle, cmngr_msg_t* msg);
static cmngr_err_t rx(const void* handle, cmngr_msg_t* msg);
static void can0_tx_task(void);
static void can0_rx_task(cmngr_msg_t* msg);
static void can1_tx_task(void);
static void can1_rx_task(cmngr_msg_t* msg);
static void can2_tx_task(void);
static void can2_rx_task(cmngr_msg_t* msg);

flexcan_id_table_t can0_id_table[7] =
{
    {.id = TYT60KW_UPDATE_050, .isExtendedFrame = false, .isRemoteFrame = false, },
    {.id = TYT60KW_UPDATE_051, .isExtendedFrame = false, .isRemoteFrame = false, },
    {.id = TYT60KW_UPDATE_222, .isExtendedFrame = false, .isRemoteFrame = false,},
    {.id = TYT60KW_UPDATE_225, .isExtendedFrame = false, .isRemoteFrame = false,},
    {.id = TYT60KW_UPDATE_507, .isExtendedFrame = false, .isRemoteFrame = false,},
    {.id = TYT60KW_UPDATE_508, .isExtendedFrame = false, .isRemoteFrame = false,},
};

flexcan_id_table_t can1_id_table[4] =
{
    {.id = FC135KW_SYS_STATUS, .isExtendedFrame = true, .isRemoteFrame = false, },
    {.id = FC135KW_SYS_H2, .isExtendedFrame = true, .isRemoteFrame = false, },
    {.id = FC135KW_SYS_WATER, .isExtendedFrame = true, .isRemoteFrame = false,},
    {.id = FC135KW_SYS_ERROR, .isExtendedFrame = true, .isRemoteFrame = false,},
};

can_message_t can_tx_msg[3][16];
can_message_t can_rx_msg[3];

s32_can_t can0 =
{
    .instance = INST_CANCOM1,
    .state = &canCom1_State,
    .user_cfg = &canCom1_InitConfig0,
};

s32_can_t can1 =
{
    .instance = INST_CANCOM2,
    .state = &canCom2_State,
    .user_cfg = &canCom2_InitConfig0,
};

s32_can_t can2 =
{
    .instance = INST_CANCOM3,
    .state = &canCom3_State,
    .user_cfg = &canCom3_InitConfig0,
};

cmngr_config_t cmngr_config =
{
    .init = init,
    .deinit = deinit,
    .filter = filter,
    .tx_message = tx,
    .rx_message = rx,
};

cmngr_t can0_manager =
{
    .handle = &can0,
    .tx_task = can0_tx_task,
    .rx_task = can0_rx_task,
};

cmngr_t can1_manager =
{
    .handle = &can1,
    .tx_task = can1_tx_task,
    .rx_task = can1_rx_task,
};

cmngr_t can2_manager =
{
    .handle = &can2,
    .tx_task = can2_tx_task,
    .rx_task = can2_rx_task,
};

void app_can_init(void)
{
    cmngr_init(&cmngr_config);

    cmngr_add(&can0_manager);
    cmngr_rx_config(&can0_manager, (cmngr_msg_t*)&can_rx_msg[0]);
    cmngr_tx_config(&can0_manager, (cmngr_msg_t*)&can_tx_msg[0][0], 16, sizeof(can_message_t));

    cmngr_add(&can1_manager);
    cmngr_rx_config(&can1_manager, (cmngr_msg_t*)&can_rx_msg[1]);
    cmngr_tx_config(&can1_manager, (cmngr_msg_t*)&can_tx_msg[1][0], 16, sizeof(can_message_t));

    cmngr_add(&can2_manager);
    cmngr_rx_config(&can2_manager, (cmngr_msg_t*)&can_rx_msg[2]);
    cmngr_tx_config(&can2_manager, (cmngr_msg_t*)&can_tx_msg[2][0], 16, sizeof(can_message_t));
}

static cmngr_err_t init(const void* handle)
{
    return can_init((s32_can_t*)handle) ? CMNGR_SUCCESS : CMNGR_ERROR;
}

static cmngr_err_t deinit(const void* handle)
{
    return can_deinit((s32_can_t*)handle) ? CMNGR_SUCCESS : CMNGR_ERROR;
}

static void filter(const void* handle)
{
    s32_can_t* can = (s32_can_t*)handle;

    if (can == &can0)
    {
        can_set_filter(can,
            &can0_id_table[0],
            sizeof(can0_id_table) / sizeof(flexcan_id_table_t));
    }
    else if (can == &can1)
    {
        can_set_filter(can,
            &can1_id_table[0],
            sizeof(can1_id_table) / sizeof(flexcan_id_table_t));
    }
}

static cmngr_err_t tx(const void* handle, cmngr_msg_t* msg)
{
    return can_tx_message((s32_can_t*)handle, *(can_message_t*)msg) ? CMNGR_SUCCESS : CMNGR_ERROR;
}

static cmngr_err_t rx(const void* handle, cmngr_msg_t* msg)
{
    return can_rx_message((s32_can_t*)handle, msg) ? CMNGR_SUCCESS : CMNGR_ERROR;
}

static void can0_tx_task(void)
{
    can_message_t message = { 0 };

    message.idt = 0;
    message.dlc = 8;

    if (time_flag.bits.t0_01s)
    {
        message.id = TYT60KW_CMD_04E;
        tyt60kw_command(&tyt60kw, (tyt60kw_command_t)message.id, &message.data.bytes[0]);
        cmngr_tx_message(&can0_manager, &message);
    }
}

static void can0_rx_task(cmngr_msg_t* msg)
{
    can_message_t* message = (can_message_t*)msg;

    tyt60kw_update(&tyt60kw, message->id, &message->data.bytes[0]);
}

static void can1_tx_task(void)
{
    can_message_t message = { 0 };

    message.idt = 1;
    message.dlc = 8;

    if (time_flag.bits.t0_10s)
    {
        message.id = FC135KW_SET_STATUS;
        fc135kw_command(&fc135kw, message.id, &message.data.bytes[0]);
        cmngr_tx_message(&can1_manager, &message);

        message.id = FC135KW_DRAIN_VALVE;
        fc135kw_command(&fc135kw, message.id, &message.data.bytes[0]);
        cmngr_tx_message(&can1_manager, &message);

        message.id = FC135KW_REMOVE_BUBBLE;
        fc135kw_command(&fc135kw, message.id, &message.data.bytes[0]);
        cmngr_tx_message(&can1_manager, &message);
    }
}

static void can1_rx_task(cmngr_msg_t* msg)
{
    can_message_t* message = (can_message_t*)msg;

    fc135kw_update(&fc135kw, message->id, &message->data.bytes[0]);
}

static void can2_tx_task(void)
{
    can_message_t message = { 0 };

    message.idt = 1;
    message.dlc = 8;

    if (time_flag.bits.t0_20s)
    {
        message.id = TYT60KW_HOST_CAN_RX_ID;
        tyt60kw_host_packet(&tyt60kw, &message.data.bytes[0]);
        cmngr_tx_message(&can2_manager, &message);

        message.id = FC135KW_HOST_CAN_RX_ID;
        fc135kw_host_packet(&fc135kw, &message.data.bytes[0]);
        cmngr_tx_message(&can2_manager, &message);
    }

    if (time_flag.bits.t0_50s)
    {
        bidc300100_request(&dcdc, BIDC300100_SYSTEM, 5);
        bidc300100_request(&dcdc, BIDC300100_ONOFF, 0);
    }
}

static void can2_rx_task(cmngr_msg_t* msg)
{
    can_message_t* message = (can_message_t*)msg;

    switch (message->id)
    {
        case TYT60KW_HOST_CAN_TX_ID:
        {
            tyt60kw_host_connect(&tyt60kw);
            tyt60kw_start(&tyt60kw, (message->data.bytes[0]));
            tyt60kw_power(&tyt60kw, message->data.bytes[2]);

            if (message->data.bytes[1] != 0)
            {
                tyt60kw_clear_error(&tyt60kw);
            }
            break;
        }
        case FC135KW_HOST_CAN_TX_ID:
        {
            fc135kw_host_connect(&fc135kw);
            fc135kw_start(&fc135kw, (message->data.bytes[0]));
            fc135kw_power(&fc135kw, message->data.bytes[2]);

            if (message->data.bytes[1] != 0)
            {
                fc135kw_clear_error(&fc135kw);
            }
            break;
        }
        default:
        {
            bidc300100_parser(&dcdc, message->id, &message->data.bytes[0], 8);
            break;
        }
    }
}
