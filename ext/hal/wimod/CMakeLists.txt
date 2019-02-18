if(CONFIG_LORA_WIMOD)
  zephyr_include_directories(.)

  zephyr_sources(
    wimod_hci_driver.c
    wimod_lorawan_api.c
    wimod_slip.c
    )
endif()
