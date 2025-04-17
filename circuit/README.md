**Circut**
<br>A1 to A4 from the Arduino control the servos. The D1 Mini uses A1 to check whether the Delorean is flying. The connection to A6 simulates the push button and there we need a resistor to prevent a short circuit. The IRL510 interrupts or releases the power to the Arduino. And D2 is always switched on when the Delorean is switched on. The D1 Mini therefore uses this to detect whether the Delorean is on or off.
<br>$${\color{red}Attention!}$$ The Arduino uses a different voltage (5V) at the PINs than the D1 Mini (3.3V). Therefore we need another resistors for protection.
<br><img src="/images/circuit_diagram.png" width="428">
<br><br>
