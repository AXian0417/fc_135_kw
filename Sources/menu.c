#include "app.h"
#include "menu.h"

typedef void(*pFunction)(void);

void menu_main_start(void);
void menu_main(void);
void menu_main_stop(void);
void menu_main_pause(void);

/*--------------------------------------------------------------------------------------------------------*/

pFunction menu_level_new = menu_main_start;
pFunction menu_level_old = 0;

/*--------------------------------------------------------------------------------------------------------*/

void menu(void)
{
    pFunction menu_level = menu_level_new;

    menu_level();

    if (time_flag.bits.t0_01s &&
        (menu_level != menu_main_start))
    {
        fc135kw_task(&fc135kw);
        tyt60kw_task(&tyt60kw);
    }
}

/*--------------------------------------------------------------------------------------------------------*/

void menu_main_start(void)
{
    enum
    {
        WAIT = 0,
        FC1_RELAY_ON,
        FC2_RELAY_ON,
        DCDC_ON,
    };

    static uint8_t step;
    static uint8_t time_count;
    static uint16_t light_tower;

    if (menu_level_old != menu_level_new)
    {
        menu_level_old = menu_level_new;
        light_tower = 0;
        time_count = 0;
        step = WAIT;
        app_logger("waiting for system startup\r\n");

        FC_TYT60KW_HOST_POWER(1);
        FC_135KW_HOST_POWER(1);
    }

    if (time_flag.bits.t0_10s)
    {
        switch (key_scan())
        {
            default:
            case 0x0000: { break; }
            case 0x0001: { key_timer_set(15); break; }
            case 0x01FF:
            {
                if (step == WAIT)
                {
                    app_logger("system startup\r\n");
                    step = FC1_RELAY_ON;
                    time_count = 0;
                    light_tower = 0;
                }
                break;
            }
        }
    }

    time_count += time_flag.bits.t1_00s;

    if (step != WAIT)
    {
        LIGHT_TOWER(time_flag.bits.t0_50s_clock, 0, 0, 0);
    }

    switch (step)
    {
        default:
        case WAIT:
        {
            if (time_flag.bits.t0_20s)
            {
                if (light_tower == 0)
                {
                    light_tower = 0x0001;
                }

                LIGHT_TOWER((light_tower & 0x0001), 0, 0, 0);
                light_tower <<= 1;
            }
            break;
        }
        case FC1_RELAY_ON:
        {
            FC_TYT60KW_HIGH_POWER_RELAY(1);
            FC_TYT60KW_HOST_POWER(1);
            time_count = 0;
            step = FC2_RELAY_ON;
            app_logger("fc1 high power relay on\r\n");
            break;
        }
        case FC2_RELAY_ON:
        {
            if (time_count > 2)
            {
                FC_135KW_HIGH_POWER_RELAY(1);
                FC_135KW_HOST_POWER(1);
                app_logger("fc2 high power relay on\r\n");
                bidc300100_set_ctrl(&dcdc, BIDC300100_ON);
                app_logger("enable dcdc\r\n");
                time_count = 0;
                step = DCDC_ON;
            }
            break;
        }
        case DCDC_ON:
        {
            if (time_count > 5)
            {
                step = WAIT;
                app_logger("enable dcdc failed\r\n");
            }
            else if (bidc300100_status(&dcdc).power_output == true)
            {
                menu_level_new = menu_main;
            }
            break;
        }
    }
}

void menu_main(void)
{
    static uint8_t idle_time;

    if (menu_level_old != menu_level_new)
    {
        menu_level_old = menu_level_new;
        idle_time = 0;

        if (fc135kw_status(&fc135kw) != FC135KW_INIT)
        {
            fc135kw_start(&fc135kw, true);
        }

        if (tyt60kw_status(&tyt60kw) != FC135KW_INIT)
        {
            tyt60kw_start(&tyt60kw, true);
        }
    }

    if (time_flag.bits.t0_10s)
    {
        switch (key_scan())
        {
            default:
            case 0x0000: { break; }
            case 0x0002: { key_timer_set(5); break; }
            case 0x02FF:
            {
                menu_level_new = menu_main_stop;
                app_logger("system shutdown\r\n");
                break;
            }
        }
    }

    if ((fc135kw_status(&fc135kw) == FC135KW_IDLE) &&
        (tyt60kw_status(&tyt60kw) == TYT60KW_IDLE))
    {
        LIGHT_TOWER(1, 0, 0, 0);
    }
    else
    {
        idle_time = 0;

        if ((fc135kw_status(&fc135kw) == FC135KW_ERROR) ||
            (tyt60kw_status(&tyt60kw) == TYT60KW_ERROR))
        {
            LIGHT_TOWER(0, 0,
                time_flag.bits.t1_00s_clock,
                time_flag.bits.t1_00s_clock);
        }
        else if ((fc135kw_status(&fc135kw) == FC135KW_RUNNING) ||
            (tyt60kw_status(&tyt60kw) == TYT60KW_RUNNING))
        {
            LIGHT_TOWER(0, 1, 0, 0);
        }
        else if ((fc135kw_status(&fc135kw) == FC135KW_PRECHARGE) ||
            (tyt60kw_status(&tyt60kw) == TYT60KW_PRECHARGE))
        {
            LIGHT_TOWER(0, time_flag.bits.t0_20s_clock, 0, 0);
        }
        else
        {
            LIGHT_TOWER(0, 0, 0, 0);
        }
    }

// TODO
// if (time_flag.bits.t1_00s)
// {
//     if (idle_time < 30)
//     {
//         if (++idle_time >= 30)
//         {
//             menu_level_new = menu_main_stop;
//             app_logger("system idle, into to stop\r\n");
//         }
//     }
// }
}

void menu_main_stop(void)
{
    static uint8_t step;
    static uint16_t timeout;

    if (menu_level_old != menu_level_new)
    {
        menu_level_old = menu_level_new;
        step = 0;
        timeout = 1800;
        fc135kw_deinit(&fc135kw);
        tyt60kw_deinit(&tyt60kw);
        app_logger("deinit fc modules\r\n");
    }

    LIGHT_TOWER(time_flag.bits.t0_50s_clock, 0, 0, 0);

    if (time_flag.bits.t0_10s)
    {
        if (timeout > 1)
        {
            timeout--;
        }

        switch (step)
        {
            default:
            case 0:
            {
                if (((fc135kw_status(&fc135kw) == FC135KW_DEINIT) &&
                    (tyt60kw_status(&tyt60kw) == TYT60KW_DEINIT)) ||
                    (timeout == 1))
                {
                    step++;
                }
                break;
            }
            case 1:
            {
                bidc300100_set_ctrl(&dcdc, BIDC300100_OFF);
                app_logger("disable dcdc\r\n");
                step++;
                timeout = 1;
                break;
            }
            case 2:
            {
                if (bidc300100_status(&dcdc).power_output == false)
                {
                    // TODO
                    // FC_TYT60KW_HIGH_POWER_RELAY(0);
                    // FC_TYT60KW_HOST_POWER(0);
                    FC_135KW_HIGH_POWER_RELAY(0);
                    FC_135KW_HOST_POWER(0);
                    menu_level_new = menu_main_start;
                    app_logger("system shutdown success\r\n");
                }
                else if (timeout == 1)
                {
                    timeout = 5;
                    bidc300100_set_ctrl(&dcdc, BIDC300100_OFF);
                }
                break;
            }
        }
    }
}

void menu_main_pause(void)
{
    if (menu_level_old != menu_level_new)
    {
        menu_level_old = menu_level_new;
        app_logger("menu_main_pause\r\n");
    }
}
