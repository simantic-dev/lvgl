idf_component_register(SRCS "main.c" REQUIRES lvgl esp_driver_spi esp_driver_gpio esp_timer)
if(DEFINED DEMO)
  target_compile_definitions(${COMPONENT_LIB} PRIVATE DEMO_${DEMO})
endif()
