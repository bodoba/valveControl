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

The assumptions is that the application interacts with a local mqtt broker.  Remote access to the system needs to  be controlled by other means. 

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

I use an [OrangPI Zero](http://www.orangepi.org/orangepizero/) with [four relais](https://www.amazon.de/AZDelivery-4-Relais-Optokoppler-Low-Level-Trigger-Arduino/dp/B078Q8S9S9/) connected to it to control magnetic valves.  To allow manual control there are also five push-buttons conncted. One for each valve and one to enable/disable the timer function.

Each button has an indicater LED to display the current state. For the relais these are connected to the same port, for the timer a seperate port is used. 

So in total 10 I/O ports are utilized. In my setup a [mcp23017](http://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf) port expander, connected over I2C is providing the IO ports. As  [wiringPi](http://wiringpi.com/) is used to control the I/O ports you can easily choose a different setup. WiringPi abstracts these HW details nicely.


## Command Line Switches

Option    |  Semantics
----------| --------------------------
-d | Increase debug level 
-f  |  Force forground mode
-s <*filename*> | Specify scedule table file
-c | Clear schedule table cache

The schedule table is initialized with the entries in the file specified with `-s`. On top of these all events present before shutdown will be restored unless the `-c` option is present.  

## System Setup

tbd

## Remote API

The app is controlled over MQTT and reports back over MQTT. 

### Change State

Topic    |  Payload  |  Semantics
---------| --------------------------- | -----------
`/YardControl/Command/Valve_[A\|B\|C\|D]` | `[ON\|OFF]` |  Turn the specified valve On or Off
`/YardControl/Command/mqttLogging`  |  `[ON\|OFF]`  |  Enable/Disable log messaged over MQTT.  Log messages are sent with topic `/YardControl/Log`.
`/YardControl/Command/Timer`  |  `[ON\|OFF]`  |  Enable/Disable timer function (*) 

### Modify Schedule Table

Topic    |  Payload  |  Semantics
---------| ----------- | -----------
`/YardControl/Command/addEvent`       |  `[A\|B\|C\|D] [ON\|OFF] hh:mm`  |  Add timer event to turn the specified valve ON or OFF at the given time. 
`/YardControl/Command/removeEvent` |  `[A\|B\|C\|D] [ON\|OFF] hh:mm`  |  Remove the timer event which turns the specified valve ON or OFF at the given time. 

### Retrieve Status

Topic    |  Payload  |  Semantics
---------| ----------- | -----------
`/YardControl/Command/Refresh`                      | *ignored*  | Trigger transmission of status.  (The status report is sent after each status change)
`/YardControl/Command/dumpScheduleTable`  | *ignored*  | Trigger transmission of current schedule table 

#### Examples

**Status**
```
>> /YardControl/Command/Refresh 0

/YardControl/State/Valve_A OFF
/YardControl/State/Valve_B OFF
/YardControl/State/Valve_C OFF
/YardControl/State/Valve_D OFF
/YardControl/State/Timer OFF
```

**Schedule Table**
```
>>  /YardControl/Command/dumpScheduleTable 0

/YardControl/ScheduleTable/Entry Valve_A ON 10:00
/YardControl/ScheduleTable/Entry Valve_A ON 10:09
/YardControl/ScheduleTable/Entry Valve_A OFF 10:15
/YardControl/ScheduleTable/Entry Valve_C ON 11:15
/YardControl/ScheduleTable/Entry Valve_C OFF 11:20
/YardControl/ScheduleTable/Entry Valve_D OFF 15:47
/YardControl/ScheduleTable/Entry Valve_D OFF 15:49
/YardControl/ScheduleTable/Entry Valve_D OFF 15:52
/YardControl/ScheduleTable/Entry Valve_D ON 23:05
/YardControl/ScheduleTable/Entry Valve_D OFF 21:27
/YardControl/ScheduleTable/Entry Valve_D OFF 20:33
/YardControl/ScheduleTable/Entry Valve_D OFF 21:45
/YardControl/ScheduleTable/Entry Valve_D ON 21:46
/YardControl/ScheduleTable/Entry Valve_D ON 21:57
/YardControl/ScheduleTable/Entry Valve_D OFF 21:58
/YardControl/ScheduleTable/Entry Valve_B ON 23:30
```

## Config File Syntax

The syntax is the same as for the `addEvent` API.  Everything after ´#´ is ignored as comment

```
#
#   Valve     State     Time
# (A/B/C/D)  (ON/OFF)  (hh:mm)

A   ON   10:00
A   ON   10:09
A   OFF  10:15

C   ON   11:15
C   OFF  11:20
```
