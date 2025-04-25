# Circuit

**Hardware**
- <img src="/images/D1MiniNodeMCU.png" width="100"><br>[ES8266 D1 mini (Clone)](https://de.aliexpress.com/item/1005006890254253.html)
- 2x 1k ohm 1/4 watt Resistor
- 1x 47k ohm 1/4 watt Resistor
- 2x 100k ~ 180k ohm 1/4 watt Resistor
- 1x IRLZ44N Mosfet
<br><br>
  
**Additional Hardware for EQ LED Mod**

- <img src="/images/AdafruitCharliePlex.jpg" width="200"><br>[Adafruid 15x7 CharliPlex LED Matrix FeatherWing Warm White](https://www.berrybase.de/adafruit-15x7-charlieplex-led-matrix-featherwing-warmweiss)
<br><br>

## Wifi Control only

A1 to A4 from the Arduino control the servos. The D1 Mini uses A1 to check whether the Delorean is flying. The connection to A6 simulates the push button and there we need a resistor to prevent a short circuit. The IRLZ44N interrupts or releases the power to the Arduino. And D2 is always switched on when the Delorean is switched on. The D1 Mini therefore uses this to detect whether the Delorean is on or off.
<br>$${\color{red}Attention!}$$ The Arduino uses a different voltage (5V) at the PINs than the D1 Mini (3.3V). Therefore we need another resistors for protection.
<br><img src="/circuit/circuit_diagram.png" width="428">
<br><br>

## With EQ LED Mod

For the EQ LED Mod you only need to connect the Adafruid CharlyPlex to 3.3V, GND, SDA and SCL from the D1 mini.
Every time the Deloren is Power on, the CharlyPlex will start the LED Animation for the Status Indicator. This Mod need more room, therefore
you need another Wall-Rear for the Deloren. Also you need anoter EQ-Defusor and EQ-Front.
All STL Files you will find [here](/stl).
<br><img src="/circuit/circuit_diagram_eq.png" width="600">
<br><br>
