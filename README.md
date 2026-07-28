# ESP32 component: AppFS

This component consists of two parts: the AppFS component itself and a bootloader modification which makes it possible to launch ESP32 firmwares dynamically from the AppFS partition.

## Integrating into an ESP-IDF project
In your project directory execute the following commands:

 - cd components; git submodule add https://github.com/badgeteam/esp32-component-appfs.git; cd ..
 - mkdir bootloader_components
 - ln -s ../components/appfs appfs
 - ln -s ../components/appfs/bootloader_main main

## Compatibility

Fully supported Xtensa chips:

- esp32
- esp32s3

Fully supported RISC-V chips:

- esp32c6
- esp32p4
- esp32c5
- esp32c3
- esp32h2

Partially supported RISC-V chips:

- esp32c61

The ESP32-C61 does not have RTC/LP RAM and thus passing an argument string to an application is not supported on this chip.

## License

For the full Apache 2.0 license text see [LICENSE](LICENSE). For licensing notices see [NOTICE](NOTICE).
