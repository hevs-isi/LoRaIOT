zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SHTC1 shtc1.c)
