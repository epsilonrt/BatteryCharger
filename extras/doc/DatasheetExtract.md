# Datasheet Extract

## ISET measurement

### CN3063

Constant Charge Current Setting and Charge Current Monitor Pin. The
charge current is set by connecting a resistor RISET from this pin to GND.
When in precharge mode, the ISET pin’s voltage is regulated to 0.2V. When in
constant charge current mode, the ISET pin’s voltage is regulated to 2V. In all
modes during charging, the voltage on ISET pin can be used to measure the
charge current as follows:
ICH = (VISET／ RISET)× 900

## CN3163

Constant Charge Current Setting and Charge Current Monitor Pin. The
charge current is set by connecting a resistor RISET from this pin to GND.
When in precharge mode, the ISET pin’s voltage is regulated to 0.12V. When
in constant charge current mode, the ISET pin’s voltage is regulated to 1.205V.
In all modes during charging, the voltage on ISET pin can be used to measure
the charge current as follows:
ICH = (VISET／ RISET)× 986

## Practical summary

- CN3063: maximum charge current is 600 mA (practical RISET about 3.0 kOhm for max current).
- CN3065: maximum charge current is 1 A (theoretical RISET 1.8 kOhm).
- CN3163: maximum charge current is 1 A (theoretical RISET 1.19 kOhm, use 1.2 kOhm).
- CN3165: maximum charge current is 1 A (theoretical RISET 1.19 kOhm, use 1.2 kOhm).

### CN3065

Constant Charge Current Setting and Charge Current Monitor Pin. The
charge current is set by connecting a resistor RISET from this pin to GND.
When in precharge mode, the ISET pin’s voltage is regulated to 0.2V. When in
constant charge current mode, the ISET pin’s voltage is regulated to 2V. In all
modes during charging, the voltage on ISET pin can be used to measure the
charge current as follows:
ICH = (VISET／ RISET)× 900

### CN3165

Constant Charge Current Setting and Charge Current Monitor Pin. The
charge current is set by connecting a resistor RISET from this pin to GND.
When in precharge mode, the ISET pin’s voltage is regulated to 0.12V. When
in constant charge current mode, the ISET pin’s voltage is regulated to 1.205V.
In all modes during charging, the voltage on ISET pin can be used to measure
the charge current as follows:
ICH = (VISET／ RISET)× 986

## NTC thermistor

### CN3063

Temperature Sense Input. Connecting TEMP pin to NTC thermistor’s
output in Lithium ion battery pack. If TEMP pin’s voltage is below 46% of
input supply voltage VIN for more than 0.15S, this means that battery’s
temperature is too high or too low, charging is suspended. If TEMP’s voltage
level is above 46% of input supply voltage for more than 0.15S, battery fault
state is released, and charging will resume.
The temperature sense function can be disabled by grounding the TEMP pin.

### CN3163

Temperature Sense Input. Connecting TEMP pin to NTC thermistor’s
output in Lithium ion battery pack. If TEMP pin’s voltage is below 45% or
above 80% of supply voltage VIN, this means that battery’s temperature is too
high or too low, charging is suspended. If TEMP’s voltage level is between
45% and 80%of supply voltage, battery fault state is released, and charging will
resume.
The temperature sense function can be disabled by grounding the TEMP pin.

### CN3065

Temperature Sense Input. Connecting TEMP pin to NTC thermistor’s
output in Lithium ion battery pack. If TEMP pin’s voltage is below 46% of
input supply voltage VIN for more than 0.15S, this means that battery’s
temperature is too high or too low, charging is suspended. If TEMP’s voltage
level is above 46% of input supply voltage for more than 0.15S, battery fault
state is released, and charging will resume.
The temperature sense function can be disabled by grounding the TEMP pin

### CN3165

Temperature Sense Input. Connecting TEMP pin to NTC thermistor’s
output in Lithium ion battery pack. If TEMP pin’s voltage is below 45% or
above 80% of supply voltage VIN, this means that battery’s temperature is too
high or too low, charging is suspended. If TEMP’s voltage level is between
45% and 80%of supply voltage, battery fault state is released, and charging will
resume.
The temperature sense function can be disabled by grounding the TEMP pin.


