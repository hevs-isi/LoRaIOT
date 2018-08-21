include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main.c)
#target_sources(app PRIVATE src/sysview.c)
zephyr_sources_ifdef(CONFIG_SEGGER_SYSTEMVIEW src/sysview.c)
zephyr_sources_ifdef(CONFIG_LORA src/lora.c)
target_sources(app PRIVATE src/sensors.c)
target_sources(app PRIVATE src/app.c)

#FILE(GLOB app_sources src/*.c)
#target_sources(app PRIVATE ${app_sources})

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/sensor/pac1934)