
idf_build_get_property(target IDF_TARGET)

set(appfs_requires spi_flash bootloader_support)
set(appfs_srcs src/appfs.c)

if(NOT BOOTLOADER_BUILD)
    set(appfs_requires ${appfs_requires} vfs)
    set(appfs_srcs ${appfs_srcs} src/appfs_vfs.c)
endif()
if(NOT BOOTLOADER_BUILD AND ${IDF_VERSION_MAJOR} GREATER 4)
    set(appfs_requires ${appfs_requires} esp_partition)
endif()
if(NOT "${target}" MATCHES "esp32")
    set(appfs_requires ${appfs_bl_requires} hal)
endif()

idf_component_register(
    SRCS ${appfs_srcs}
    INCLUDE_DIRS "include"
    REQUIRES ${appfs_requires}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -DPROJECT_NAME="${CMAKE_PROJECT_NAME}")
