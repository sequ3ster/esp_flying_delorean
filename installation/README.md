**Firmware**
<br>The released firmware is for the D1 Mini (Clone). But maybe it works also for other ESP8266 Boards.
Feel free to build your own version with the Arduino IDE. Then you need also to upload the Data folder to LittleFS.
<br><br>

**Installation**
<br>For the firmware installation I can recommend the online tool from Espressif. [https://espressif.github.io/esptool-js/](https://espressif.github.io/esptool-js/)
<br><img src="/images/esptool.png" width="300"><br>
Use 115200 baud, address 0x0000 for the latest firmware and address 0x200000 for the littleFS image. The littleFS image contains images for the web frontend. $${\color{red}Attention!}$$ Flashing the littleFS image deletes the MQTT configuration.
You only need to flash the littleFS image during the initial installation. Each firmware update can be installed OTA and you do not have to update the images again.
<br><img src="/images/esptool_latest.png" width="300">
<img src="/images/esptool_littlefs.png" width="300">
<br><br>

**Access Point Mode**
<br>When both Images are flashed to the ESP8266, you need to restart the Controller via RST button or power cycle it. Please connect your phone to the Wifi “Flying Delorean”. Then open your browser and go to 192.168.4.1. There you can connect the Wifi Controller to your Wifi network. You can also configure the MQTT Config. Every time you want to change the Wifi settings, you have to go back to AP mode. You will find a button for this in the [configuration menu](/images/Screenshot_20250416_210028_Chrome.jpg).
<br><img src="/images/Screenshot_20250416_210753_Chrome.jpg" width="300">
<img src="/images/Screenshot_20250416_210802_Chrome.jpg" width="300">
<br><br>
