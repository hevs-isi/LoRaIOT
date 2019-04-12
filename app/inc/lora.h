#ifndef _LORA_H
#define _LORA_H

#ifdef __cplusplus
extern "C"
{
#endif

void lora_init(void);
void lora_off(void);
void disable_uart(void);
void enable_uart(void);

#ifdef __cplusplus
}
#endif

#endif /* _LORA_H */
