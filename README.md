# ESP Flying Delorean
## Wifi Controller for the Flying DELOREAN V2 Animatronic from Did3D.

[Didier Picard](https://www.did3d.fr) has created an impressive 3D model of a [flying Delorean from Back to the Future Part II](https://www.cgtrader.com/3d-print-models/miniatures/vehicles/flying-delorean-v2-hq-1-8-scale-530mm-3d-print-model).
This model has animatronic and can be controlled via a button on the bottom. The LEDs, servos and sound module are controlled by an Arduino Nano. As there is already a lot to control, all the outputs of the Arduino Nano are used. This leaves no room for expansion.

But there is a solution: a second microcontroller.
For Didier's first Delorean model, an EQ LED panel MOD for the status indicator was already implemented by [Craig Hancock](https://www.facebook.com/share/p/18tshCDeAy/), with a second Arduino Nano. I also wanted to implement this mod for the Flying Delorean. And I had the idea of using an ESP8266 with Wifi, to control the Arduino. So the idea for this project “ESP Flying Deloren” was born.

**Functions**
- Wifi connection with [website](/images/Screenshot_20250405_155759_Chrome.jpg) for control 
- Switching the Delorean on/off
- Changing the scenes
- Changing the mode
- EQ LED mod for the status indicator
- Communication via MQTT (with Home Assistant Discovery Support)

**Circut**
<br><img src="/images/circuit_diagram.png" width="300">

**Screenshots**
<br><img src="/images/Screenshot_20250405_155759_Chrome.jpg" height="300">

**Hardware**
- <img src="/images/D1MiniNodeMCU.png" height="300"><br>[ES8266 D1 mini](ES8266 D1 mini: https://de.aliexpress.com/item/1005006890254253.html)
- <img src="/images/AdafruitCharliePlex.jpg" width="300"><br>[Adafruid 15x7 CharliPlex LED Matrix FeatherWing Warm White](https://www.berrybase.de/adafruit-15x7-charlieplex-led-matrix-featherwing-warmweiss)
- 1x 1k ohm 1/4 watt Resistor
- 1x 180K ohm 1/4 watt Resistor
- 1x IRL510 Mosfet
