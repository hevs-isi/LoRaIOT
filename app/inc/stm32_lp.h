#ifndef STM32_LP_H
#define STM32_LP_H

#ifdef __cplusplus
extern "C"
{
#endif

void stm32_sleep(u32_t duration);
void stm32_swd_off(void);
void stm32_swd_on(void);
void psu_5v(u32_t enable);
void psu_ind(u32_t enable);
void psu_charge(u32_t enable);
void psu_cpu_hp(u32_t enable);
void lp_init(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32_LP_H */
