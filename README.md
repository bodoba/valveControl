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

* Android App
An android app is availble that integrates with this server software. See https://github.com/bodoba/valveControlApp

### Limitations

**No security mechnisms  are provided**

The assumptions is that the application interacts with a local mqtt broker.  Remote access to the system needs to  be controlled by other means. 

**Only minimal safety mechanisms are provided** 

The assumption is that the IO ports work as designed. There is no second source to control the actual status of the relais compared to the state of the output ports.

The only mechnism to limit possible harm are:
* Relais are shut off by a timeout after the last ON coommand
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
-p<*prefix*> | Use  "/<'prefix'>" to prefix all incoming and outgoing MQTT messages. Defaults to "YardControl"

The schedule table is initialized with the entries in the file specified with `-s`. On top of these all events present before shutdown will be restored unless the `-c` option is present.  

Use `-p` if you run multiple instances. This allows to address every instance individually.  The `/YardControl/...`  prefix shown in all examples here will have to be changed to the specified prefix.

## System Setup

![](/Doc/SystemArchitecture.png)

tbc

## Remote API

The app is controlled over MQTT and reports back over MQTT. 

### Change State

Topic    |  Payload  |  Semantics
---------| --------------------------- | -----------
`/YardControl/Command/Valve_[A\|B\|C\|D]` | `[ON\|OFF]` |  Turn the specified valve On or Off
`/YardControl/Command/Timer`  |  `[ON\|OFF]`  |  Enable/Disable timer function (*) 

### Modify Schedule Table

Topic    |  Payload  |  Semantics
---------| ----------- | -----------
`/YardControl/Command/addEvent`       |  `[A\|B\|C\|D] [ON\|OFF] hh:mm`  |  Add timer event to turn the specified valve ON or OFF at the given time. 
`/YardControl/Command/removeEvent` |  `[A\|B\|C\|D] [ON\|OFF] hh:mm`  |  Remove the timer event which turns the specified valve ON or OFF at the given time. 
`/YardControl/Command/setValveTimeout`  |  Integer Value  |  Valve will be shut off after <timeout> seconds after last `ON` action even without explicit `OFF` command . Defaults to 1200. Changed value is persisted over restarts  

### Retrieve Status

Topic    |  Payload  |  Semantics
---------| ----------- | -----------
`/YardControl/Command/Refresh`                      | *ignored*  | Trigger transmission of status.  (The status report is sent after each status change)
`/YardControl/Command/dumpScheduleTable`  | *ignored*  | Trigger transmission of current schedule table 
`/YardControl/Command/getValveTimeout`      | *ignored*  | get current timeout setting  
`/YardControl/Command/getEventHistory`      | *ignored*  | Trigger transmission ot the last time each valve has been switched on and off 

### Debugging

You can change the log level and retrieve log messages:

Topic    |  Payload  |  Semantics
---------| ----------- | -----------
`/YardControl/Command/mqttLogging`  |  `[ON\|OFF]`  |  Enable/Disable log messaged over MQTT.  Log messages are sent with topic `/YardControl/Log`.
`/YardControl/Command/setLogLevel` | `[EMERGENCY\|ALERT\|CRITICAL\|ERROR\|WARNING\|NOTICE\|INFO\|DEBUG]` | set log level (see also [syslog(2)](https://linux.die.net/man/2/syslog)) 
`/YardControl/Command/getLogLevel`| *ignored* | Returns the currently used log level 
`/YardControl/Command/printLog`| *ignored* |  Trigger transmission of the last hundred log messages
`/YardControl/Command/exit`| *ignored* |  Turn all vales off and quit the application. (Most useful if a watchdog restarts it)

### Examples

**Status**
```
> mosquitto_pub -t "/YardControl/Command/Refresh" -m "0"
```
Messages sent:
```
/YardControl/State/Valve_A OFF
/YardControl/State/Valve_B OFF
/YardControl/State/Valve_C OFF
/YardControl/State/Valve_D OFF
/YardControl/State/Timer ON
```

**Schedule Table**
```
> mosquitto_pub -t "/YardControl/Command/dumpScheduleTable" -m "0"
```
Messages sent:
```
/YardControl/ScheduleTable/Entry Valve_A ON 10:00
/YardControl/ScheduleTable/Entry Valve_A ON 10:09
/YardControl/ScheduleTable/Entry Valve_A OFF 10:15
/YardControl/ScheduleTable/Entry Valve_B ON 23:30
/YardControl/ScheduleTable/Entry Valve_B OFF 23:35
/YardControl/ScheduleTable/Entry Valve_C ON 11:15
/YardControl/ScheduleTable/Entry Valve_C OFF 11:20
/YardControl/ScheduleTable/Entry Valve_D ON 23:05
/YardControl/ScheduleTable/Entry Valve_D OFF 23:10
```

**Logging**
```
> mosquitto_pub -t "/YardControl/Command/printLog" -m "1"
```
Messages sent:
```
/YardControl/Log 000121 2020-05-03 16:09:19 <NOTICE> Set log level to NOTICE
/YardControl/Log 000120 2020-05-03 16:09:19 <INFO> Reveived MQTT Command: /YardControl/Command/setLogLevel -> NOTICE
/YardControl/Log 000119 2020-05-03 16:07:53 <DEBUG> writeState( Valve_C, FALSE )
/YardControl/Log 000118 2020-05-03 16:07:53 <DEBUG> readState( Valve_C ) -> TRUE
/YardControl/Log 000117 2020-05-03 16:07:53 <NOTICE> Valve_C forced OFF by Valve_B
/YardControl/Log 000116 2020-05-03 16:07:53 <NOTICE> Valve_B toggled to ON
/YardControl/Log 000115 2020-05-03 16:07:53 <DEBUG> writeState( Valve_B, TRUE )
/YardControl/Log 000114 2020-05-03 16:07:53 <DEBUG> readState( Valve_B ) -> FALSE
/YardControl/Log 000113 2020-05-03 16:07:53 <NOTICE> MQTT command: Switch Valve_B ON
/YardControl/Log 000112 2020-05-03 16:07:53 <INFO> Matched button: Valve_B
/YardControl/Log 000111 2020-05-03 16:07:53 <INFO> Reveived MQTT Command: /YardControl/Command/Valve_B -> ON
/YardControl/Log 000110 2020-05-03 16:07:52 <NOTICE> Valve_C toggled to ON
/YardControl/Log 000109 2020-05-03 16:07:52 <DEBUG> writeState( Valve_C, TRUE )
/YardControl/Log 000108 2020-05-03 16:07:52 <DEBUG> readState( Valve_C ) -> TRUE
/YardControl/Log 000107 2020-05-03 16:07:52 <NOTICE> MQTT command: Switch Valve_C ON
/YardControl/Log 000106 2020-05-03 16:07:52 <INFO> Matched button: Valve_C
/YardControl/Log 000105 2020-05-03 16:07:52 <INFO> Reveived MQTT Command: /YardControl/Command/Valve_C -> ON
/YardControl/Log 000104 2020-05-03 16:07:51 <NOTICE> Valve_B toggled to OFF
/YardControl/Log 000103 2020-05-03 16:07:51 <DEBUG> writeState( Valve_B, FALSE )
/YardControl/Log 000102 2020-05-03 16:07:51 <DEBUG> readState( Valve_B ) -> FALSE
[...]
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
