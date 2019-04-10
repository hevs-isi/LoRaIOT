#ifndef SAVED_CONFIG_H
#define SAVED_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct saved_config_t
{
	uint32_t version;
	uint32_t boot_count;
	uint32_t revision;
	uint8_t _reserved[48+2*64];
	uint32_t crc;
};

void saved_config_init(void);

int saved_config_read(struct saved_config_t *config);
int saved_config_save(struct saved_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* SAVED_CONFIG_H */
