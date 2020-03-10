# valveControl

This application provides a mqtt based remote control for irrigation valves to automate watering your garden.

### Features
* Control four relais to open/close [magentic valves](https://www.amazon.de/Hunter-PGV-101-Magnetventil-PGV-101-mmB/dp/B001P0ESSE/)
    * Auto-Off after ten minutes (safe-guard)
    * Only one valve can be ON at a given time (constant water pressure ) 

* Local control
    * ON/OFF button for each valve
    * Button to Enable/Disable timer
    * Status LEDs to see open valves/active timer

* Timer control
   * Turn ON/OFF at given time
   * Schedule read from config file
   * Schedule can be changes remotely
 
* Remote control
    * Simple MQTT used messages to commuicate with device
    * Turn valves ON/OFF
    * Enable/Disable timer function
    * Get device status
    * Add/Remove events to schedule table
    * Get Schedule table with all timer events

### Limitations

**No security mechnisms  are provided**

The assumptions is that the application interacts with a local mqtt broker.  Remote access to the system needs to  controlled by other means. 

**Only minimal safety mechanisms are provided** 

The assumption is that the IO ports work as designed. There is no second source to control the actual status of the relais compared to the state of the output ports.

The only mechnism to limit possible harm are:
* Relais are shut off 10 minutes after the last ON coommand
* Only one relais at a time is set to ON
* Port states are refreshed every 10 seconds
* All relais are set to OFF state at startup 

**Please watchdog the application**

In case it stops and a relais is ON the connected valve may remain open. This may lead to damage or increases water costs.

## Hardware

I use a [OrangPI Zero](http://www.orangepi.org/orangepizero/) with [four relais](https://www.amazon.de/AZDelivery-4-Relais-Optokoppler-Low-Level-Trigger-Arduino/dp/B078Q8S9S9/) connected to it to control magnetic valves.  To allow manual control there are also five push-buttons conncted. One for each valve and one to enable/disable the timer function.

Each button has an indicater LED to display the current state. For the relais these are connected to the same port, for the timer a seperate port is used. 

So in total 10 I/O ports are utilized. In my setup a [mcp23017](http://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf) port expander, connected over I2C is providing the IO ports. As  [wiringPi](http://wiringpi.com/) is used to control the I/O ports you can easily choose a different setup. WiringPi abstracts these HW details nicely.

## System Setup

tbd

## Remote API
tdb

### Change State
Topic    |  Payload  |  Semantics
---------| ----------- | -----------
`/YardControl/Command/Valve_X` | `[ON\|OFF]` |  Turn Valve <*X*>  ON/OFF.  *X* needs to be A, B, C or D 
`/YardControl/Command/mqttLogging` | `ON` or `OFF` |  Enable/Disable log messaged over MQTT.  Log messages are sent with topic `/YardControl/Log`.

### Modify Schedule Table
Topic    |  Payload  |  Semantics
---------| ----------- | -----------
`/YardControl/Command/addEvent`       |   |  Turn Valve *X*  ON/OFF.  *X* needs to be A, B, C or D 
`/YardControl/Command/removeEvent` | `1` or `0` |  Turn Valve *X*  ON/OFF.  *X* needs to be A, B, C or D 

### Retrieve Status
tdb
