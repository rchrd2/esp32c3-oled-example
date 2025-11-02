# Example Project for ESP32-C3 with the OLED screen

Based on this: https://www.lucadentella.it/en/2017/10/30/esp32-25-display-oled-con-u8g2/

Help from @nemethricsi01 as well. And Cursor

The u8g2 code is downloaded as a zip from https://github.com/olikraus/u8g2/. It is stored in the repo for convenience.

# How to build/run/etc

This is based off of the espressif VS Code plugin. However, you can use the CLI as well.

```
$ source $IDF_PATH/export.sh
$ idf.py build
$ idf.py flash
```
