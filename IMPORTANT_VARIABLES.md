## deviceStats[7]
|Index | |
|-|-|
|0 |Last reset time|
|1 |Total wanted time (s)|
|2 |Desired current (mA)|
|3 |Actual current (mA)|
|4 |Load voltage (mV)|
|5 |Output enabled flag|
|6 |Resistance of the load (ohm)|


## correctionVariables[4][2]:
Here the calibration variables are saved.

||**Device 1**|**Device 2** | 
| ---|---|----|
|1| A0 correction| 
|2| A1 correction|
|3| current_sense_resistor|
|4| load voltage correction|

## PIN Definition
| **Pin Nr.** | Meaning|
|----------|-|
|A0 | Output Voltage (DAC)|
|A1 | Current Sense Resistor (ADC)|
|A2 | Load Voltage Negative (ADC)|
|A3 | Load Voltage Positive (ADC)|
|A4 | SDA (I2C)|
|A5 | SCL (I2C)|
|2  | Encoder A|
|3  | Encoder B|
|6  | Encoder Button|
|8  | Small Button|

