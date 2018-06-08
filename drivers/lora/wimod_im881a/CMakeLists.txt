if(CONFIG_LORA_IM881A)
  zephyr_include_directories(./)

  zephyr_sources(
    im881a.c
    )
endif()
