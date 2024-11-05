#ifndef _define_io_h_
#define _define_io_h_

/*--------------------------------------------------------------------------------------------------------*/

#define PA6_OUT(n)                              PINS_DRV_WritePin(PTA, 6, n)    // 10A_RELAY(K17)    
#define PA8_OUT(n)                              PINS_DRV_WritePin(PTA, 8, n)    // 10A_RELAY        // ERR 被接去 GND 
#define PA9_OUT(n)                              PINS_DRV_WritePin(PTA, 9, n)    // 10A_RELAY(K3)    
#define PA17_OUT(n)                             PINS_DRV_WritePin(PTA, 17, n)   // 30A_RELAY(K8)    
#define PB5_OUT(n)                              PINS_DRV_WritePin(PTB, 5, n)    // 10A_RELAY(K21)    
#define PC1_OUT(n)                              PINS_DRV_WritePin(PTC, 1, n)    // 10A_RELAY(K7)    
#define PC2_OUT(n)                              PINS_DRV_WritePin(PTC, 2, n)    // SYSTEM LED
#define PC3_OUT(n)                              PINS_DRV_WritePin(PTC, 3, n)    // 10A_RELAY(K1)    
#define PD6_OUT(n)                              PINS_DRV_WritePin(PTD, 6, n)    // 光耦合 out 5v(U10A)              
#define PD11_OUT(n)                             PINS_DRV_WritePin(PTD, 11, n)   // 10A_RELAY(K4)    // ERR 
#define PD12_OUT(n)                             PINS_DRV_WritePin(PTD, 12, n)   // 10A_RELAY(K5)    // ERR 有燈無動作
#define PD13_OUT(n)                             PINS_DRV_WritePin(PTD, 13, n)   // 10A_RELAY(K19)
#define PD14_OUT(n)                             PINS_DRV_WritePin(PTD, 14, n)   // 10A_RELAY(K20)
#define PD16_OUT(n)                             PINS_DRV_WritePin(PTD, 16, n)   // 10A_RELAY(K13)
#define PD17_OUT(n)                             PINS_DRV_WritePin(PTD, 17, n)   // 10A_RELAY(K12)
#define PE0_OUT(n)                              PINS_DRV_WritePin(PTE, 0, n)    // 10A_RELAY(K2)
#define PE1_OUT(n)                              PINS_DRV_WritePin(PTE, 1, n)    // 10A_RELAY (K10)
#define PE2_OUT(n)                              PINS_DRV_WritePin(PTE, 2, n)    // 10A_RELAY(K15) 
#define PE3_OUT(n)                              PINS_DRV_WritePin(PTE, 3, n)    // 外部電 MOS(Q11)  // OC MOS 待測
#define PE7_OUT(n)                              PINS_DRV_WritePin(PTE, 7, n)    // 30A_RELAY(K16)
#define PE9_OUT(n)                              PINS_DRV_WritePin(PTE, 9, n)    // 10A_RELAY(K23)
#define PE12_OUT(n)                             PINS_DRV_WritePin(PTE, 12, n)   // 10A_RELAY(K9) 
#define PE13_OUT(n)                             PINS_DRV_WritePin(PTE, 13, n)   // 10A_RELAY(K11) 
#define PE14_OUT(n)                             PINS_DRV_WritePin(PTE, 14, n)   // 10A_RELAY(K22) 

/*--------------------------------------------------------------------------------------------------------*/

#define PB3_IN()                                ((PINS_DRV_ReadPins(PTB) >> (3)) & 0x01)    // DI6
#define PC8_IN()                                ((PINS_DRV_ReadPins(PTC) >> (8)) & 0x01)    // DI3
#define PC9_IN()                                ((PINS_DRV_ReadPins(PTC) >> (9)) & 0x01)    // DI4
#define PC12_IN()                               ((PINS_DRV_ReadPins(PTC) >> (12)) & 0x01)   // 板上 key1
#define PC13_IN()                               ((PINS_DRV_ReadPins(PTC) >> (13)) & 0x01)   // 板上 key2
#define PC15_IN()                               ((PINS_DRV_ReadPins(PTC) >> (15)) & 0x01)   // DI5
#define PD8_IN()                                ((PINS_DRV_ReadPins(PTD) >> (8)) & 0x01)    // DI8
#define PE15_IN()                               ((PINS_DRV_ReadPins(PTE) >> (15)) & 0x01)   // DI1
#define PE16_IN()                               ((PINS_DRV_ReadPins(PTE) >> (16)) & 0x01)   // DI2

/*--------------------------------------------------------------------------------------------------------*/

#define PD15_PWM(ps)                            pwm_duty(INST_FLEXTIMER_PWM0, 0, (ps))  // 外部電 MOS(Q15)  // OC MOS 待測
#define PD0_PWM(ps)                             pwm_duty(INST_FLEXTIMER_PWM0, 1, (ps))  // S_MOS(Q1)        // OC MOS 待測
#define PE8_PWM(ps)                             pwm_duty(INST_FLEXTIMER_PWM0, 2, (ps))  // 24v_MOS(Q8) 
#define PA1_PWM(ps)                             pwm_duty(INST_FLEXTIMER_PWM1, 0, (ps))  // S_MOS(Q3)        // OC MOS 待測
#define PC0_PWM(ps)                             pwm_duty(INST_FLEXTIMER_PWM1, 1, (ps))  // 24v_MOS(Q9)
#define PD5_PWM(ps)                             pwm_duty(INST_FLEXTIMER_PWM2, 0, (ps))  // 光耦合 out 5v(U10B)
#define PE10_PWM(ps)                            pwm_duty(INST_FLEXTIMER_PWM2, 1, (ps))  // 外部電 MOS(Q12)  // OC MOS 待測
#define PE11_PWM(ps)                            pwm_duty(INST_FLEXTIMER_PWM2, 2, (ps))  // 外部電 MOS(Q14)  // OC MOS 待測
#define PA2_PWM(ps)                             pwm_duty(INST_FLEXTIMER_PWM3, 0, (ps))  // 24v_MOS(Q4)
#define PB9_PWM(ps)                             pwm_duty(INST_FLEXTIMER_PWM3, 1, (ps))  // 0 ~ 10v          // 待測
#define PB10_PWM(ps)                            pwm_duty(INST_FLEXTIMER_PWM3, 2, (ps))  // S_MOS(Q2)        // OC MOS 待測

/*--------------------------------------------------------------------------------------------------------*/

#define SYS_RUN_LIGHT(n)                        PC2_OUT(n) 

#define BUTTON_ON()                             !PE15_IN()
#define BUTTON_OFF()                            !PE16_IN()
#define KEY_1()                                 PC12_IN()
#define KEY_2()                                 PC13_IN()

#define LIGHT_TOWER(g, y, r, bz)                PE12_OUT(!g);    \
                                                PA9_OUT(!y);     \
                                                PD16_OUT(!r);    \
                                                PD17_OUT(!bz)

#define FC_TYT60KW_HIGH_POWER_RELAY(n)          PB5_OUT(!n)      
#define FC_TYT60KW_HOST_POWER(n)                PD13_OUT(!n)
#define FC_TYT60KW_POWER(n)                     PE0_OUT(!n)
#define FC_TYT60KW_IGN(n)                       PD14_OUT(!n)
#define FC_TYT60KW_FAN_H(ps)                    PE10_PWM(ps);   \
                                                PE11_PWM(ps)
#define FC_TYT60KW_FAN_L(ps)                    PD15_PWM(ps)

#define FC_135KW_HIGH_POWER_RELAY(n)            PC1_OUT(!n)
#define FC_135KW_HOST_POWER(n)                  PE9_OUT(!n)
#define FC_135KW_POWER(n)                       PE1_OUT(!n)
#define FC_135KW_IGN(n)                         PD12_OUT(!n)

#endif
