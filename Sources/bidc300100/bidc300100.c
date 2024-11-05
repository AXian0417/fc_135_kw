#include "bidc300100.h"
#include <assert.h>

/**
 * @brief ID 組合填充
 *
 * @param pf 數據幀類型
 * @param ps 接收方地址
 * @param sa 發送方地址
 *
 * @return uint32_t 組合後的 ID
 *
 * @note - |       PF      |      PS      |      SA     |
 * @note   | bit23 ~ bit16 | bit15 ~ bit8 | bit7 ~ bit0 |
 */
static uint32_t id_fill(bidc300100_frame_t pf, uint8_t ps, uint8_t sa)
{
    return (uint32_t)((((uint32_t)pf) << 16) | (((uint32_t)ps) << 8) | sa);
}

/**
 * @brief BIDC300100 DCDC 初始化
 *
 * @param dcdc DCDC 物件
 * @param addr 設定物件地址 (0 ~ 31)
 * @param tx 數據發送接口
 *
 * @return true 初始化成功
 * @return false 初始化失敗
 */
bool bidc300100_init(bidc300100_t* dcdc, uint8_t addr, bidc300100_tx_t tx)
{
    assert(dcdc);

    if (addr <= 0x1FU)
    {
        dcdc->addr = (addr | 0x20U);
        dcdc->tx = tx;

        dcdc->status.power_output = false;

        return true;
    }

    return false;
}

/**
 * @brief BIDC300100 DCDC 狀態
 *
 * @param dcdc DCDC 物件
 *
 * @return bidc300100_status_t
 */
bidc300100_status_t bidc300100_status(bidc300100_t* dcdc)
{
    assert(dcdc);

    return dcdc->status;
}

/**
 * @brief BIDC300100 DCDC 設定控制
 *
 * @param dcdc DCDC 物件
 * @param ctrl 要控制的類型
 *
 * @return true 設定成功
 * @return false 設定失敗
 */
bool bidc300100_set_ctrl(bidc300100_t* dcdc, bidc300100_ctrl_t ctrl)
{
    const uint8_t command[BIDC300100_CTRL_MAX][2] =
    {
        [BIDC300100_ON] = { 32, 0x55U },
        [BIDC300100_OFF] = { 32, 0xAAU },
        [BIDC300100_RESET] = { 29, 0xAAU },
    };

    uint32_t id;
    bidc300100_data_t buffer = { 0 };

    assert(dcdc);

    if (dcdc->tx && (ctrl < BIDC300100_CTRL_MAX))
    {
        id = id_fill(BIDC300100_FRAME_SETUP, dcdc->addr, BIDC300100_HOST_ADDR);

        buffer.data.bytes[2] = 0;
        buffer.data.bytes[3] = command[ctrl][0];    /* order */     // motorola

        buffer.data.bytes[0] = 0;
        buffer.data.bytes[1] = command[ctrl][1];    /* set value */ // motorola

        buffer.data.lbyte[1] = 0;
        dcdc->tx(id, &buffer.data.bytes[0], 8);

        return true;
    }

    return false;
}

/**
 * @brief BIDC300100 請求數據
 *
 * @param dcdc DCDC 物件
 * @param request 要請求的類型
 * @param mult 0: 單一數據請求, 其他: 請求數據的數量
 *
 * @return true 設定成功
 * @return false 設定失敗
 *
 * @note mult > 0 時，可請求多個數據，數據會從帶入的 request 這個類型
 *       開始按順序請求 mult 個 (順序與 bidc300100_request_t 無關，需
 *       查看產品 Datasheet)
 */
bool bidc300100_request(bidc300100_t* dcdc, bidc300100_request_t request, uint8_t mult)
{
    uint32_t id;
    bidc300100_data_t buffer = { 0 };

    assert(dcdc);

    if (dcdc->tx)
    {
        id = id_fill(BIDC300100_FRAME_QUERY, dcdc->addr, BIDC300100_HOST_ADDR);

        if (mult == 0)
        {
            /*
                查詢單一數據
                buffer.data.bytes[2] + buffer.data.bytes[3] = order = 要查詢的數據編號
                其他 = 0
             */
            buffer.data.bytes[2] = 0;
            buffer.data.bytes[3] = request; // motorola

            buffer.data.wbyte[0] = 0;
            buffer.data.wbyte[3] = 0;
        }
        else
        {
            /*
                查詢多個數據
                buffer.data.wbyte[1] = order = 0
                buffer.data.bytes[2] + buffer.data.bytes[3] = srart order = 起使數據編號
                buffer.data.bytes[6] + buffer.data.bytes[7] = len = mult
             */
            buffer.data.wbyte[1] = 0;

            buffer.data.bytes[0] = 0;
            buffer.data.bytes[1] = request; // motorola

            buffer.data.bytes[6] = 0;
            buffer.data.bytes[7] = mult;// motorola
        }

        buffer.data.wbyte[2] = 0;

        dcdc->tx(id, &buffer.data.bytes[0], 8);

        return true;
    }

    return false;
}

/**
 * @brief BIDC300100 數據解析器
 *
 * @param dcdc DCDC 物件
 * @param id 收到的 ID
 * @param data 收到的數據
 * @param size 收到數據的長度
 *
 * @return true 解析成功
 * @return false 解析失敗
 */
bool bidc300100_parser(bidc300100_t* dcdc, uint32_t id, uint8_t* data, uint8_t size)
{
    bidc300100_data_t* buffer;
    bidc300100_request_t request;

    assert(dcdc);
    assert(data);

    if ((size == 8) &&
        (id == id_fill(BIDC300100_FRAME_RESPONSE, BIDC300100_HOST_ADDR, dcdc->addr)))
    {
        buffer = (bidc300100_data_t*)data;
        request = (bidc300100_request_t)((buffer->data.bytes[2] << 8) | buffer->data.bytes[3]);

        switch (request)
        {
            case BIDC300100_ONOFF:
            {
                if (buffer->data.bytes[1] == 0x55)
                {
                    dcdc->status.power_output = true;
                }
                else if (buffer->data.bytes[1] == 0xAA)
                {
                    dcdc->status.power_output = false;
                }
                break;
            }
            default: { return false; }
        }

        return true;
    }

    return false;
}
