# ESP32 Arduino electrometer with NeoPixel and web plot

This is a simple circuit that detects changes in static electric field.  Readings are displayed on a NeoPixel ring and (optionally) on a web plot over wifi.  Move your hand near it and the pixel ring will spin.  If you're standing on carpet with shoes on, lifting your foot will make it go crazy.  It can detect lightning and cats.

![Schematic](schematic.png)
![Breadboard](breadboard.jpg)

## Theory of operation

## Parts list

* [QtPy ESP32-S2](https://www.adafruit.com/product/5325)
* [MCP6007 - low leakage op-amp](https://www.digikey.com/en/products/detail/microchip-technology/MCP6007-E-MS/12807177)
* [ADS1219 - 24-bit ADC with 60Hz reject](https://www.digikey.com/en/products/detail/texas-instruments/ADS1219IPWR/9597825)
* [NeoPixel Ring, 16 pixels](https://www.adafruit.com/product/1463)
* Resistors: 3 * 10k, 100k, 2 * 10M
* Capacitors: 10uF, 100pF

## Firmware building

    arduino-cli --fqbn esp32:esp32:adafruit_qtpy_esp32s2 -p /dev/ttyACM0 compile -u --warnings all

    # https://stackoverflow.com/questions/71952177/firebeetle-esp32-and-mklittlefs-parameters
    cp ~/.arduino15/packages/esp32/hardware/esp32/2.0.11/tools/partitions/default.csv sensor-web-plot/partitions.csv
    grep spiffs sensor-web-plot/partitions.csv
        spiffs,   data, spiffs,  0x290000,0x160000,
    ~/.arduino15/packages/esp32/tools/mklittlefs/3.0.0-gnu12-dc7f933/mklittlefs -c ./sensor-web-plot/data -p 256 -b 4096 -s $((0x160000)) spiffs.bin
    python3 ~/.arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool.py --chip esp32s2 --port "/dev/ttyACM0" --baud 921600  --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect $((0x290000)) spiffs.bin

