#ifndef _LORA_H
#define _LORA_H

#ifdef __cplusplus
extern "C"
{
#endif

void lora_init(void);
void disable_uart();
void enable_uart();

#ifdef __cplusplus
}
#endif

#endif /* _LORA_H */
