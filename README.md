<img src="/EspFlyingDelorean/Data/BTTF2.webp" width="250" alt="# ESP Flying Delorean">

[Circuit](/circuit)<br><br>
[Firmware Installation](/installation)<br><br>
[3D Printing STL](/stl) for EQ LED Mod<br><br>

## Wifi Controller for the Flying DELOREAN V2 Animatronic from Did3D.
<img src="/images/flying_delorean.jpg" width="250">

<br>[Didier Picard](https://www.did3d.fr) has created an impressive 3D model of a [flying Delorean from Back to the Future Part II](https://www.cgtrader.com/3d-print-models/miniatures/vehicles/flying-delorean-v2-hq-1-8-scale-530mm-3d-print-model).
This model has animatronic and can be controlled via a button on the bottom. The LEDs, servos and sound module are controlled by an Arduino Nano. As there is already a lot to control, all the outputs of the Arduino Nano are used. This leaves no room for expansion.

That's why I thought it would be a good idea to connect an ESP8266 to the Arduino. This way the Delorean can be switched on and off via Wifi and the scenes and moods can also be switched. In addition, there are enough free connections to add the EQ LED mod from the Delorean V1.<br><br>

**Functions**
- Wifi connection with website for control 
- Switching the Delorean on/off
- Changing the scenes
- Changing the mode
- Communication via MQTT (with Home Assistant Discovery Support)
- EQ LED mod for the status indicator<br><img src="/images/Animation.gif" width="180">

$${\color{red}WARNING!}$$: The car must never be switched on unless it is resting on its wheels in road mode,
otherwise there is a risk of breaking the gears.
This is why the on/off button on the website and the mqtt power switch do not work when the Delorean is flying.
<br><br>

<br><br>**Frontend**
<br><img src="/images/Screenshot_20250416_210002_Chrome.jpg" width="250">
<img src="/images/Screenshot_20250416_210016_Chrome.jpg" width="250">
<img src="/images/Screenshot_20250416_210028_Chrome.jpg" width="250">
<br><br>

**Home Assistant**
<br><img src="/images/Screenshot_20250417_094744_Home Assistant.jpg" width="300">
<br><br>
