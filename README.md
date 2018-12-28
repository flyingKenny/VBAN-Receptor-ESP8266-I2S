# Wireless sound card with EESP8266 and VBAN protocol
The target of this project is to build a WiFi speaker, which is directly fed via UDP stream.
Application are
* reducing long analog wires 
* multiroom speakers for home automation
* using multiply devices with one speaker e.g. two PC's and RaspberryPI media center

## Used resources
* [ESP8266 core for Arduino](https://github.com/esp8266/Arduino)
* [VBAN protocol open-source implementation](https://github.com/quiniouben/vban) with release [v2.1.0](https://github.com/quiniouben/vban/releases/tag/v2.1.0)
* [ESP8266 WiFi Connection manager with web captive portal](https://github.com/tzapu/WiFiManager)


## Wiring

|ESP8266 / nodemcu|to|I2S DAC PCM5102|
|---:|:---:|:---|
|GPIO2 / D4|->  |LCK|
|GPIO15 / D8|-> |BCK|
|GPIO2 / RX|->| DIN|
|GND / G|-> |GND|
|3V3 / 3V|-> |VIN|
| | |SCK -> GND|

![Image](https://github.com/flyingKenny/VBAN-Receptor-ESP8266-I2S/blob/master/VBAN_ESP8266_I2SDAC_PCM5102.png "image")

## Arduino settings
* CPU Frequency: 80MHz
* Flash Size: 4M (3MSPIFFS)
* lwIP Variant: v2 Higher Bandwidth

## Usage
Use [VB-AUDIO Software](https://www.vb-audio.com/index.htm) to generate a VBAN Stream. I recommend the [Banana Software](https://www.vb-audio.com/Voicemeeter/banana.htm).
Setup the VBAN Stream:
* Source: e.g. BUS A1
* Stream Name: Stream1 "Stream1 is hard programmed in software"
* IP Address To: IP of your ESP device
* Port: 6980
* Sample Rate: 11024Hz to 64000Hz works (22050Hz to 48000Hz is recommended)
* CH: 1 or 2 is supported
* Format: PCM 16 bits
* Net Quality: Optimal to Very Solw works on my side
