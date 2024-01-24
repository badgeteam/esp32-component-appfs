
idf_component_register(SRC_DIRS .
        REQUIRES log spi_flash bootloader_support esp_partition
        INCLUDE_DIRS . include )

target_compile_options(${COMPONENT_LIB} PRIVATE -DPROJECT_NAME="${CMAKE_PROJECT_NAME}")
