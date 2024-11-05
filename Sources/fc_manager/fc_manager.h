#ifndef _fc_manager_h_
#define _fc_manager_h_

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

typedef enum fc_status
{
    FC_INIT = 0,
    FC_IDLE,
    FC_PRECHARGE,
    FC_RUNNING,
    FC_SHUTDOWN,
    FC_ERROR,
    FC_STATUS_MAX,
} fc_status_t;

typedef struct fuel_cell_host
{
    struct
    {
        bool lg;
        bool ly;
        bool lr;
        bool bz;
    } light_tower;

    bool connect;

} fuel_cell_host_t;

// TODO
typedef struct fcmngr_config
{
    const char* name;
    void (*log)(const char* str, uint16_t size);

} fcmngr_config_t;







typedef struct fc_config
{
    void(*init)(void* handle);
    void(*task)(void* handle);

} fc_config_t;



typedef struct fuel_cell
{
    void* handle;
    fc_config_t* config;
    fuel_cell_host_t* host;



    uint8_t step;
    uint16_t time_count;
    fc_status_t status;
    fc_status_t status_old;







    struct fuel_cell* next;
} fuel_cell_t;

#endif
