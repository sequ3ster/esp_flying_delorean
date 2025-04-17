# ESP Flying Delorean
## Wifi Controller for the Flying DELOREAN V2 Animatronic from Did3D.

[Didier Picard](https://www.did3d.fr) has created an impressive 3D model of a [flying Delorean from Back to the Future Part II](https://www.cgtrader.com/3d-print-models/miniatures/vehicles/flying-delorean-v2-hq-1-8-scale-530mm-3d-print-model).
This model is animatronic and can be controlled via a button on the bottom. The LEDs, servos and sound module are controlled by an Arduino Nano. As there is already a lot to control, all the outputs of the Arduino Nano are used. This leaves no room for expansion.

In Didier's first Delorean model, an EQ LED panel MOD for the status indicator was already implemented, but with a second Arduino Nano. Because this version has also no room for expansion. I also wanted to implement this mod for the Flying Delorean and I had the idea of using an ESP8266 to control the Arduino. So the idea for this project “ESP Flying Deloren” with EQ LED Mod was born.
<br><br>

**Functions**
- Wifi connection with website for control 
- Switching the Delorean on/off
- Changing the scenes
- Changing the mode
- EQ LED mod for the status indicator
- Communication via MQTT (with Home Assistant Discovery Support)

$${\color{red}WARNING!}$$: The car must never be switched on unless it is resting on its wheels in road mode,
otherwise there is a risk of breaking the gears.
This is why the on/off button and the mqtt power switch do not work when the Delorean is flying.
<br><br>

**Circut**
<br>A1 to A4 from the Arduino control the servos. The D1 Mini uses A1 to check whether the Delorean is flying. The connection to A6 simulates the push button and there we need a resistor to prevent a short circuit. The IRL510 interrupts or releases the power to the Arduino. And D2 is always switched on when the Delorean is switched on. The D1 Mini therefore uses this to detect whether the Delorean is on or off.
<br>$${\color{red}Attention!}$$ The Arduino uses a different voltage (5V) at the PINs than the D1 Mini (3.3V). Therefore we need another resistors for protection.
<br><img src="/images/circuit_diagram.png" width="428">
<br><br>

**Hardware**
- <img src="/images/D1MiniNodeMCU.png" width="150"><br>[ES8266 D1 mini (Clone)](https://de.aliexpress.com/item/1005006890254253.html)
- 1x 1k ohm 1/4 watt Resistor
- 2x 180K ohm 1/4 watt Resistor
- 1x IRL510 Mosfet
- Additional Hardware for EQ LED Mod
  - <img src="/images/AdafruitCharliePlex.jpg" width="300"><br>[Adafruid 15x7 CharliPlex LED Matrix FeatherWing Warm White](https://www.berrybase.de/adafruit-15x7-charlieplex-led-matrix-featherwing-warmweiss)
<br><br>

**EQ LED Mod Circut**
<br>For the EQ LED Mod you only need to connect the Adafruid CharlyPlex to 3.3V, GND, SDA and SCL from the D1 mini.
Every time the Deloren is Power on, the CharlyPlex will start the LED Animation for the Status Indicator. This Mod need more room, therefore
you need another Wall-Rear for the Deloren. Also you need anoter EQ-Defusor and EQ-Front.
All STL Files you will get from Google Drive. Search in your purchased Files for "Link to GOOGLEDRIVE_FlyDELOREAN2_Did3D(UPDATE).txt".
<br><img src="/images/Animation.gif">
<br><img src="/images/circuit_diagram_eq.png" width="600">
<br><br>

**Firmware**
<br>The released firmware is for the D1 Mini (Clone). But maybe it works also for other ESP8266 Boards.
Feel free to build your own version with the Arduino IDE. Then you need also to upload the Data folter to LittleFS.
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

**Frontend**
<br><img src="/images/Screenshot_20250416_210002_Chrome.jpg" width="300">
<img src="/images/Screenshot_20250416_210016_Chrome.jpg" width="300">
<img src="/images/Screenshot_20250416_210028_Chrome.jpg" width="300">
<br><br>

**Home Assistant**
<br><img src="/images/Screenshot_20250417_094744_Home Assistant.jpg" width="300">
<br><br>
