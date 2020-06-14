/* *********************************************************************************** */
/*                                                                                     */
/*  Copyright (c) 2020 by Bodo Bauer <bb@bb-zone.com>                                  */
/*                                                                                     */
/*  This program is free software: you can redistribute it and/or modify               */
/*  it under the terms of the GNU General Public License as published by               */
/*  the Free Software Foundation, either version 3 of the License, or                  */
/*  (at your option) any later version.                                                */
/*                                                                                     */
/*  This program is distributed in the hope that it will be useful,                    */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                     */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      */
/*  GNU General Public License for more details.                                       */
/*                                                                                     */
/*  You should have received a copy of the GNU General Public License                  */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.              */
/* *********************************************************************************** */
#include <stdbool.h>
#include <time.h>

#ifndef valveControl_h
#define valveControl_h

typedef struct button_t {
    const char *name;
    int        pinButton;
    int        pinLED;
    bool       state;
    int        lastReading;
    bool       radio;
    time_t     timestamp;
    time_t     lastOn;
    time_t     lastOff;
} button_t;

extern button_t pushButton[];

/* ----------------------------------------------------------------------------------- *
 * Export some functions
 * ----------------------------------------------------------------------------------- */
void setButtonState(int button, bool state);

/* ----------------------------------------------------------------------------------- *
 * Default values
 * ----------------------------------------------------------------------------------- */
#define PID_FILE           "/var/run/valvecontrol.pid"
#define CACHE_FILE         "/var/cache/valvecontrol/scheduletable"
#define MQTT_INPUT_TOPIC   "/YardControl/Command/#"                // needs to end in "#"
#define MQTT_BROKER_HOST   "localhost"
#define MQTT_BROKER_PORT   1883

/* ----------------------------------------------------------------------------------- *
 * MPC23017 IO extender
 * ----------------------------------------------------------------------------------- */
#define ADDR_IOEXT_0   0x20
#define PINBASE_0        64

/* ----------------------------------------------------------------------------------- *
 * output pins for valve control (LED & Relais)
 * ----------------------------------------------------------------------------------- */
#define VALVE_A       PINBASE_0+0   // control valve A
#define VALVE_B       PINBASE_0+1   // control valve B
#define VALVE_C       PINBASE_0+2   // control valve C
#define VALVE_D       PINBASE_0+3   // control valve D

/* ----------------------------------------------------------------------------------- *
 * Output pins for control LEDs
 * ----------------------------------------------------------------------------------- */
#define LED_S1        PINBASE_0+4   // sequence 1 selected
#define LED_S0        PINBASE_0+5   // sequence 0 selected
#define LED_RUN       PINBASE_0+7   // selected sequence is running
#define LED_AUTO      PINBASE_0+6   // automatic mode enabled

/* ----------------------------------------------------------------------------------- *
 * input pins for push buttons
 * ----------------------------------------------------------------------------------- */
#define BUTTON_A      PINBASE_0+8   // switch valve A
#define BUTTON_B      PINBASE_0+9   // switch valve B
#define BUTTON_C      PINBASE_0+10  // switch valve C
#define BUTTON_D      PINBASE_0+11  // switch valve D

#define BUTTON_RUN    PINBASE_0+12  // run selected sequence
#define BUTTON_AUTO   PINBASE_0+13  // toggle automatic mode
#define BUTTON_SELECT PINBASE_0+14  // select between sequence 1 and 2
#define NC_1_7        PINBASE_0+15  // not connected

#endif /* valveControl_h */
