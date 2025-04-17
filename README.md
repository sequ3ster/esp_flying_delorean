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
- Communication via MQTT (with Home Assistant Discovery Support)
- EQ LED mod for the status indicator<br><img src="/images/Animation.gif" width="180">

$${\color{red}WARNING!}$$: The car must never be switched on unless it is resting on its wheels in road mode,
otherwise there is a risk of breaking the gears.
This is why the on/off button and the mqtt power switch do not work when the Delorean is flying.
<br><br>

[3D Printing STL](/stl)

[Circuit](/circuit)

[Firmware Installation](/installation)


**Frontend**
<br><img src="/images/Screenshot_20250416_210002_Chrome.jpg" width="300">
<img src="/images/Screenshot_20250416_210016_Chrome.jpg" width="300">
<img src="/images/Screenshot_20250416_210028_Chrome.jpg" width="300">
<br><br>

**Home Assistant**
<br><img src="/images/Screenshot_20250417_094744_Home Assistant.jpg" width="300">
<br><br>
