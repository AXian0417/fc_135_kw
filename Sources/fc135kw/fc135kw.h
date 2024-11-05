#ifndef _fc135kw_h_
#define _fc135kw_h_

#include <stdint.h>
#include <stdbool.h>

typedef enum fc135kw_status
{
    FC135KW_INIT = 0,
    FC135KW_IDLE,
    FC135KW_PRECHARGE,
    FC135KW_RUNNING,
    FC135KW_SHUTDOWN,
    FC135KW_ERROR,
    FC135KW_DEINIT,
    FC135KW_STATUS_MAX,
} fc135kw_status_t;

typedef enum fc135kw_io
{
    FC135KW_IO_24V_POWER = 0,
    FC135KW_IO_IGN,
} fc135kw_io_t;

typedef enum fc135kw_command
{
    FC135KW_SET_STATUS = 0x18F00010,        /** 0x18F00010 設定狀態 */
    FC135KW_DRAIN_VALVE = 0x18F00070,       /** 0x18F00070 設定 purge 時間 */
    FC135KW_REMOVE_BUBBLE = 0x18F000C0,     /** 0x18F000C0 設定打水排氣泡 */
} fc135kw_command_t;

typedef enum fc135kw_update
{
    FC135KW_SYS_STATUS = 0x18FC0010,
    FC135KW_SYS_H2 = 0x18FC0040,
    FC135KW_SYS_WATER = 0x18FC0050,
    FC135KW_SYS_ERROR = 0x120BB001,
} fc135kw_update_t;

typedef union fc135kw_data
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
} fc135kw_data_t;

typedef struct fc135kw_light_tower
{
    bool lg;
    bool ly;
    bool lr;
    bool bz;
} fc135kw_light_tower_t;

typedef union fc135kw_flag
{
    struct
    {
        uint8_t start : 1;
        uint8_t deinit : 1;
        uint8_t power_on_sig : 1;
        uint8_t power_on : 1;
        uint8_t ign_key : 1;
        uint8_t emerg_stop : 1;
        uint8_t clear_error : 1;
        uint8_t remove_bubble : 1;
    } bits;

    uint8_t all;

} fc135kw_flag_t;

typedef struct fc135kw_config
{
    const char* name;
    void (*log)(const char* str, uint16_t size);
    uint16_t(*io)(fc135kw_io_t io, uint16_t value);
    void (*light_tower)(fc135kw_light_tower_t tower);
} fc135kw_config_t;

typedef struct fc135kw
{
    uint16_t power_setup;           /** 功率設定(0.01x, 最少 2000 (20 KW)) */
    uint8_t system_status;          /** 系統狀態 */
    uint16_t H2_pressure;           /** 進氣壓力(0.1x, 單位 kPa) */
    uint16_t H2_src_pressure;       /** 氣源壓力(0.1x, 單位 kPa) */
    int8_t water_temp;              /** 目前水溫(0 = -40 度) */
    int8_t water_temp_target;       /** 水溫目標(0 = -40 度) */
    uint8_t error;
    uint16_t error_code;

    uint8_t step;
    uint32_t time_count;            /** basic = 10 ms */
    uint16_t update_timeout;
    uint16_t host_timeout;

    fc135kw_flag_t flag;
    fc135kw_status_t status;
    fc135kw_status_t status_old;
    fc135kw_light_tower_t light_tower;
    fc135kw_config_t* config;
} fc135kw_t;

void fc135kw_init(fc135kw_t* fc, fc135kw_config_t* config);
void fc135kw_deinit(fc135kw_t* fc);
void fc135kw_task(fc135kw_t* fc);
fc135kw_status_t fc135kw_status(fc135kw_t* fc);
void fc135kw_start(fc135kw_t* fc, bool enable);
void fc135kw_clear_error(fc135kw_t* fc);
void fc135kw_error_stop(fc135kw_t* fc);
void fc135kw_emerg_stop(fc135kw_t* fc);
void fc135kw_remove_bubble(fc135kw_t* fc, bool enable);
void fc135kw_host_connect(fc135kw_t* fc);
void fc135kw_host_packet(fc135kw_t* fc, uint8_t data[8]);
void fc135kw_command(fc135kw_t* fc, fc135kw_command_t type, uint8_t data[8]);
void fc135kw_update(fc135kw_t* fc, fc135kw_update_t type, uint8_t data[8]);
void fc135kw_power(fc135kw_t* fc, uint16_t power_kw);

#endif
