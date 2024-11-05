#ifndef _toyota60kw_h_
#define _toyota60kw_h_

#include <stdint.h>
#include <stdbool.h>

typedef enum tyt60kw_status
{
    TYT60KW_INIT = 0,
    TYT60KW_IDLE,
    TYT60KW_PRECHARGE,
    TYT60KW_RUNNING,
    TYT60KW_SHUTDOWN,
    TYT60KW_ERROR,
    TYT60KW_DEINIT,
    TYT60KW_STATUS_MAX,
} tyt60kw_status_t;

typedef enum tyt60kw_io
{
    TYT60KW_IO_POWER = 0,
    TYT60KW_IO_IGN,
    TYT60KW_IO_FAN_H,
    TYT60KW_IO_FAN_L,
} tyt60kw_io_t;

typedef enum tyt60kw_st_sp
{
    TYT60KW_OFF = 0,
    TYT60KW_START,
    TYT60KW_STOP,
    TYT60KW_PURGE_STOP,
    TYT60KW_EMERG_STOP,
} tyt60kw_st_sp_t;

typedef enum tyt60kw_fc_status
{
    TYT60KW_STOPPED = 0,
    TYT60KW_START_PROCESSING,
    TYT60KW_POWER_SUPPLY,
    TYT60KW_STOP_PROCESSING,
    TYT60KW_STOP_FINISH,
} tyt60kw_fc_status_t;

typedef enum tyt60kw_command
{
    TYT60KW_CMD_04E = 0x04E,        /** EV_HV_1 , 10ms cycle */
    TYT60KW_CMD_215 = 0x215,        /** EV_HV_2 , 100ms cycle */
} tyt60kw_command_t;

typedef enum tyt60kw_update
{
    TYT60KW_UPDATE_050 = 0x050,
    TYT60KW_UPDATE_051 = 0x051,
    TYT60KW_UPDATE_222 = 0x222,
    TYT60KW_UPDATE_225 = 0x225,
    TYT60KW_UPDATE_507 = 0x507,
    TYT60KW_UPDATE_508 = 0x508,
} tyt60kw_update_t;

typedef union tyt60kw_data
{
    struct
    {
        uint8_t b0 : 1;
        uint8_t b1 : 1;
        uint8_t b2 : 1;
        uint8_t b3 : 1;
        uint8_t b4 : 1;
        uint8_t b5 : 1;
        uint8_t b6 : 1;
        uint8_t b7 : 1;
    } bits[8];

    uint8_t bytes[8];
    uint16_t wbyte[4];
    uint32_t lbyte[2];
} tyt60kw_data_t;

typedef struct tyt60kw_light_tower
{
    bool lg;
    bool ly;
    bool lr;
    bool bz;
} tyt60kw_light_tower_t;

typedef union tyt60kw_flag
{
    struct
    {
        uint8_t start : 1;
        uint8_t deinit : 1;
        uint8_t power_on : 1;
        uint8_t ign_key : 1;
        uint8_t emerg_stop : 1;
        uint8_t clear_error : 1;
        uint8_t relay_off : 1;
        uint8_t relay_status : 1;
        // uint8_t relay_reliability : 1;   
        uint8_t : 8;
    } bits;

    uint16_t all;

} tyt60kw_flag_t;

typedef struct tyt60kw_config
{
    const char* name;
    void (*log)(const char* str, uint16_t size);
    uint16_t(*io)(tyt60kw_io_t io, uint16_t value);
    void (*light_tower)(tyt60kw_light_tower_t tower);
} tyt60kw_config_t;

typedef struct tyt60kw
{
    uint8_t power_setup;            /** 功率設定 (100x, W) */
    uint8_t power_max;              /** 功率限制 (KW) */
    uint16_t pcu_voltage;           /** 功率控制器的電壓 (v)*/
    uint8_t pcu_temp;               /** 功率控制器溫度 */
    uint8_t w_temp_fco;             /** 電堆輸出溫度 */

    tyt60kw_st_sp_t start_command;
    tyt60kw_fc_status_t fc_status;
    uint8_t error;
    uint16_t error_code;

    uint8_t step;
    uint32_t time_count;
    uint16_t cooling_time;
    uint16_t update_timeout;
    uint16_t host_timeout;
    uint8_t cooling_fan_speed_ps_h;
    uint8_t cooling_fan_speed_ps_l;

    tyt60kw_flag_t flag;
    tyt60kw_status_t status;
    tyt60kw_status_t status_old;
    tyt60kw_light_tower_t light_tower;
    tyt60kw_config_t* config;
} tyt60kw_t;

void tyt60kw_init(tyt60kw_t* fc, tyt60kw_config_t* config);
void tyt60kw_deinit(tyt60kw_t* fc);
void tyt60kw_task(tyt60kw_t* fc);
tyt60kw_status_t tyt60kw_status(tyt60kw_t* fc);
void tyt60kw_start(tyt60kw_t* fc, bool enable);
void tyt60kw_clear_error(tyt60kw_t* fc);
void tyt60kw_error_stop(tyt60kw_t* fc);
void tyt60kw_emerg_stop(tyt60kw_t* fc);
void tyt60kw_host_connect(tyt60kw_t* fc);
void tyt60kw_host_packet(tyt60kw_t* fc, uint8_t data[8]);
void tyt60kw_command(tyt60kw_t* fc, tyt60kw_command_t type, uint8_t data[8]);
void tyt60kw_update(tyt60kw_t* fc, tyt60kw_update_t type, uint8_t data[8]);
void tyt60kw_power(tyt60kw_t* fc, uint16_t power_kw);

#endif
