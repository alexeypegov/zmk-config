if(CONFIG_LIPO_BATTERY_NRF_VDDH)
  zephyr_library()
  zephyr_library_sources(drivers/battery/lipo_battery_nrf_vddh.c)
endif()
