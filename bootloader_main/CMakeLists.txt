
idf_build_get_property(target IDF_TARGET)

idf_component_register(
    SRCS "bootloader_start.c" "appfs_flashfunctions_wrapper.c"
    REQUIRES bootloader bootloader_support appfs spi_flash esp_app_format
)

# Use the default linker scripts
idf_build_get_property(scripts BOOTLOADER_LINKER_SCRIPT)
target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")

target_link_libraries(${COMPONENT_LIB} INTERFACE 
    "-Wl,--wrap=bootloader_flash_read"
    "-Wl,--wrap=bootloader_mmap"
    "-Wl,--wrap=bootloader_munmap"
    "-Wl,--wrap=bootloader_console_deinit"
)
