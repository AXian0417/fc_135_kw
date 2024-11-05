#ifndef _bidc300100_h_
#define _bidc300100_h_

#include "stdint.h"
#include "stdbool.h"

#define BIDC300100_HOST_ADDR                    (0x0FU)

typedef void (*bidc300100_tx_t)(uint32_t id, uint8_t* data, uint8_t size);

typedef struct bidc300100_status
{
    bool power_output;
    bool reset;
} bidc300100_status_t;

typedef enum bidc300100_ctrl
{
    BIDC300100_ON = 0,
    BIDC300100_OFF,
    BIDC300100_RESET,
    BIDC300100_CTRL_MAX,
} bidc300100_ctrl_t;

typedef enum
{
    BIDC300100_FRAME_RESPONSE = 0x72U,  /** 0x72 查詢後的響應幀 */
    BIDC300100_FRAME_SETUP = 0x73U,     /** 0x73 設置幀 */
    BIDC300100_FRAME_QUERY = 0x74U,     /** 0x74 查詢幀 */
} bidc300100_frame_t;

typedef enum bidc300100_request
{
    BIDC300100_SYSTEM = 0x02U,
    BIDC300100_ONOFF = 0x20U,
} bidc300100_request_t;

typedef struct bidc300100_can
{
    union
    {
        uint8_t bytes[8];
        uint16_t wbyte[4];
        uint32_t lbyte[2];
    } data;

} bidc300100_data_t;

typedef struct bidc300100
{
    uint8_t addr;
    bidc300100_tx_t tx;
    bidc300100_status_t status;
} bidc300100_t;

bool bidc300100_init(bidc300100_t* dcdc, uint8_t addr, bidc300100_tx_t tx);
bidc300100_status_t bidc300100_status(bidc300100_t* dcdc);
bool bidc300100_set_ctrl(bidc300100_t* dcdc, bidc300100_ctrl_t ctrl);
bool bidc300100_request(bidc300100_t* dcdc, bidc300100_request_t request, uint8_t mult);
bool bidc300100_parser(bidc300100_t* dcdc, uint32_t id, uint8_t* data, uint8_t size);

#endif
