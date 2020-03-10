# valveControl

This application provides a mqtt based remote control for irrigation valves to automate watering your garden.

* Control four relais to open/close [magentic valves](https://www.amazon.de/Hunter-PGV-101-Magnetventil-PGV-101-mmB/dp/B001P0ESSE/)
    * Auto-Off after ten minutes (safe-guard)
    * Only one valve can be ON at a given time (constant water pressure ) 

* Local control
    * ON/OFF button for each valve
    * Button to Enable/Disable timer
    * Status LEDs to see open valves/active timer

* Timer control
   * Turn ON/OFF at given time (up to 100 timer events can be set)
   * Schedule read from config file
   * Schedule can be changes remotely
 
* Remote control
    * Simple MQTT used messages to commuicate with device
    * Turn valves ON/OFF
    * Enable/Disable timer function
    * Get device status
    * Add/Remove events to schedule table
    * Get Schedule table with all timer events

## Hardware

I use a [OrangPI Zero](http://www.orangepi.org/orangepizero/) with four relais connected to it to control magnetic valves.  To allow manual control there are also five push-buttons conncted. One for each valve and one to enable/disable the timer function.

Each button has an indacet LED to display the current state. For the relais these are connected to the same port, for the timer a seperate port is used. 

So in total 10 I/O ports are utilized. In my setup a [mcp23017](http://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf) port expander, connected over I2C is providing the IO ports. As  [wiringPi](http://wiringpi.com/) is used to control the I/O ports you can easily choose a different setup. WiringPi abstracts these HW details nicely.

## System setup

tbd

## Commands

tdb
