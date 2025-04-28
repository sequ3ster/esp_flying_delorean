# Circuit

**Hardware**
- <img src="/images/D1MiniNodeMCU.png" width="100"><br>[ES8266 D1 mini (Clone)](https://de.aliexpress.com/item/1005006890254253.html)
- 2x 1k ohm 1/4 watt Resistor
- 2x 10k ohm 1/4 watt Resistor
- 1x 180k ohm 1/4 watt Resistor
- 1x 220k ohm 1/4 watt Resistor
- 1x BC547C Transistor
- 1x IRLZ44N Mosfet
<br><br>
  
**Additional Hardware for EQ LED Mod**

- <img src="/images/AdafruitCharliePlex.jpg" width="200"><br>[Adafruid 15x7 CharliPlex LED Matrix FeatherWing Warm White](https://www.berrybase.de/adafruit-15x7-charlieplex-led-matrix-featherwing-warmweiss)
<br><br>

## Only EQ LED Mod

For the EQ LED Mod you only need to connect the Adafruid CharlyPlex to 3.3V, GND, SDA and SCL from the D1 mini. Connect also RX to Ground and A0 to 3.3V. 
<br>$${\color{red}Attention!}$$ Remove RX from Ground for falshing Firmware.
Every time the Deloren is Power on, the CharlyPlex will start the LED Animation for the Status Indicator. This Mod need more room, therefore
you need another Wall-Rear for the Deloren. Also you need anoter EQ-Defusor and EQ-Front.
All STL Files you will find [here](/stl).
<br><img src="/circuit/circuit_diagram_eq_only.png" width="600">
<br><br>

## Wifi Control only

The D1 Mini uses A1 (Ft Servo) to check whether the Delorean is flying. The connection to A6 simulates the push button and there we need a resistor to prevent a short circuit. The IRF9540 interrupts or releases the power to the Arduino. The D1 Mini is also connecte to the Drain from the Mosfet, to detect whether the Delorean is on or off. 
<br>$${\color{red}Attention!}$$ The Arduino uses a different voltage (5V) at the PINs than the D1 Mini (3.3V). Therefore we need another resistors for protection.
<br><img src="/circuit/circuit_diagram.png" width="428">
<br><br>

## Wifi Control with EQ LED Mod

The same as with the two individual mods. Except that A0 is not connected to 3.3V, but to the drain of the mosfet. RX remains open and is not connected to ground.
<br><img src="/circuit/circuit_diagram_eq.png" width="600">
<br><br>
