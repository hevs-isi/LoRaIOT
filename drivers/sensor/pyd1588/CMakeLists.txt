zephyr_library()

zephyr_library_sources_ifdef(CONFIG_PYD1588 pyd1588.c)
zephyr_library_sources_ifdef(CONFIG_PYD1588 pyd1588_trigger.c)
