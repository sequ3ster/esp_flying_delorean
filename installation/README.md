# Firmware Installation

**Firmware**
<br>The [released firmware](https://github.com/sequ3ster/esp_flying_delorean/releases/latest/) is for the D1 Mini (Clone). But maybe it works also for other ESP8266 Boards.
When you like to use another Controller, then feel free to build your own version with the Arduino IDE. Don't forget to upload the Data folder via [Arduino IDE to LittleFS](https://forum.arduino.cc/t/littlefs-data-upload-in-arduino-2-x-x/1174393). 
<br><br>

**Installation D1 Mini**
<br>For the firmware installation I can recommend the online tool from Espressif. [https://espressif.github.io/esptool-js/](https://espressif.github.io/esptool-js/)
<br><img src="/images/esptool.png" width="300"><br>
Use 115200 baud, flash address 0x0000 for the [latest firmware](https://github.com/sequ3ster/esp_flying_delorean/releases/latest/esp8266_D1_Mini_EspFlyingDelorean.bin) and flash address 0x200000 for the [littleFS image](https://github.com/sequ3ster/esp_flying_delorean/releases/latest/esp8266_D1_Mini_EspFlyingDelorean.littleFS_0x200000.bin). The littleFS image contains pictures for the web frontend. $${\color{red}Attention!}$$ You only need to flash the littleFS image during the initial installation. Each firmware update can be installed OTA and you do not have to update the littleFS image again. Flashing the littleFS image after installation and configuration will deletes your MQTT configuration.
<table><tr><td><a href="https://github.com/sequ3ster/esp_flying_delorean/releases/latest/esp8266_D1_Mini_EspFlyingDelorean.bin">Firmware 0x0000</a></td>
<td><a href="https://github.com/sequ3ster/esp_flying_delorean/releases/latest/esp8266_D1_Mini_EspFlyingDelorean.littleFS_0x200000.bin">littleFS 0x200000</a></td></tr>
<tr><td><img src="/images/esptool_latest.png" width="300"></td><td><img src="/images/esptool_littlefs.png" width="300"></td></tr>
</table>

<br><br>

**Access Point Mode**
<br>When both Images are flashed to the ESP8266, you need to restart the Controller via RST button or power cycle it. Please connect your phone to the Wifi “Flying Delorean”. Then open your browser and go to 192.168.4.1. There you can connect the Wifi Controller to your Wifi network. You can also configure the MQTT Config. Every time you want to change the Wifi settings, you have to go back to AP mode. You will find a button for this in the [configuration menu](/images/Screenshot_20250416_210028_Chrome.jpg).
<br><img src="/images/Screenshot_20250416_210753_Chrome.jpg" width="300">
<img src="/images/Screenshot_20250416_210802_Chrome.jpg" width="300">
<br><br>
