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

/* ----------------------------------------------------------------------------------- *
 * System includes
 * ----------------------------------------------------------------------------------- */
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>

/* ----------------------------------------------------------------------------------- *
 * Add on libraries
 * ----------------------------------------------------------------------------------- */
#include <mosquitto.h>

/* ----------------------------------------------------------------------------------- *
 * HASI
 * ----------------------------------------------------------------------------------- */
#include <wiringPi.h>
#include <mcp23017.h>

/* ----------------------------------------------------------------------------------- *
 * Project includes
 * ----------------------------------------------------------------------------------- */
#include "valveControl.h"
#include "mqttGateway.h"
#include "logging.h"
#include "daemon.h"
#include "persistState.h"
#include "scheduler.h"

/* ----------------------------------------------------------------------------------- *
 * Some globals we can't do without... ;)
 * ----------------------------------------------------------------------------------- */
int    debug        = 0;                 // debug level
bool   use_cache    = true;              // cache schedule table
bool   foreground   = false;             // run in foreground, not as daemon
char   *mqttPrefix;                      // all messages, in and out, start with /<prefix>/...
int    valveMax     = 1200;              // shut valves off after <valveTimeout> seconds unless a new command arrived

/* ----------------------------------------------------------------------------------- *
 * Prototypes
 * ----------------------------------------------------------------------------------- */
int  main(int argc, char *argv[]);
void mainLoop(void);
void setupIO (void);
void pollButtons(void);
void setButtonState(int button, bool state);
void publishButtonState(void);
void valveTimeOut(int max);
void mqttCommandCB(char *payload, int payloadlen, char *topic, void *user_data);
void setIoStates ( void );
void dumpEventHistory( void );

/* ----------------------------------------------------------------------------------- *
 * Definition of the pushbuttons
 * ----------------------------------------------------------------------------------- */
button_t pushButton[] = {
    /* name      pinButton   pinLED    state  lastReading radio timestamp lastOn lastOff*/

    { "Valve_A", BUTTON_A,   VALVE_A,  false, 0,          true,  (time_t) 0, (time_t) 0, (time_t) 0 },
    { "Valve_B", BUTTON_B,   VALVE_B,  false, 0,          true,  (time_t) 0, (time_t) 0, (time_t) 0 },
    { "Valve_C", BUTTON_C,   VALVE_C,  false, 0,          true,  (time_t) 0, (time_t) 0, (time_t) 0 },
    { "Valve_D", BUTTON_D,   VALVE_D,  false, 0,          true,  (time_t) 0, (time_t) 0, (time_t) 0 },
    { "Timer",   BUTTON_RUN, LED_RUN,  false, 0,          false, (time_t) 0, (time_t) 0, (time_t) 0 },
   // end marker
    { NULL,      -1,         -1,       false, 0,          true,  (time_t) 0, (time_t) 0, (time_t) 0 }
};

#define INDEX_TIMER 4

/* ----------------------------------------------------------------------------------- *
 * MQTT callbacks
 * ----------------------------------------------------------------------------------- */
static mqttIncoming_t subscriptions[] = {
    {MQTT_INPUT_TOPIC, &mqttCommandCB, (void*)NULL },
    {NULL, NULL, NULL}
};

/* ----------------------------------------------------------------------------------- *
 * save guard valves
 * Enforce timeout of 'Radio' buttons after given time
 * ----------------------------------------------------------------------------------- */
void valveTimeOut ( int max ) {
    time_t minStart = time(NULL) - max;
    int index = 0;

    while ( pushButton[index].name ) {
        if ( pushButton[index].radio && pushButton[index].state && pushButton[index].timestamp < minStart ) {
            writeLog(LOG_WARNING, "%s forced OFF after %d sec timeout", pushButton[index].name, max );
            setButtonState( index, false);
            publishButtonState();
        }
        index++;
    }
}

/* ----------------------------------------------------------------------------------- *
 * write button states to OI ports
 * ----------------------------------------------------------------------------------- */
void setIoStates ( void ) {
    int index = 0;
    while ( pushButton[index].name ) {
        digitalWrite (pushButton[index].pinLED, pushButton[index].state ? HIGH : LOW);
        index++;
    }
}

/* ----------------------------------------------------------------------------------- *
 * Setup IO ports
 * ----------------------------------------------------------------------------------- */
void setupIO ( void ) {
    // initialize wiring PI and attached IO extender
    wiringPiSetup () ;
    mcp23017Setup (PINBASE_0, ADDR_IOEXT_0);

    int index = 0;
    while ( pushButton[index].name ) {
        pinMode(pushButton[index].pinLED,    OUTPUT);
        pinMode(pushButton[index].pinButton, INPUT);
        pullUpDnControl(pushButton[index].pinButton, PUD_UP) ;
        index++;
    }
    setIoStates();
}

/* ----------------------------------------------------------------------------------- *
 * poll Buttons
 * ----------------------------------------------------------------------------------- */
void pollButtons(void) {
    int index = 0;
    
    while ( pushButton[index].name ) {
        int newReading = digitalRead(pushButton[index].pinButton);

        // if there has been a change
        if ( newReading != pushButton[index].lastReading ) {
            pushButton[index].lastReading = newReading;
            // toggle state
            if ( newReading == 0 ) {
                setButtonState(index, pushButton[index].state ? false : true);
                publishButtonState();
            }
        }
        index++;
   }
}

/* ----------------------------------------------------------------------------------- *
 * Set button state
 * ----------------------------------------------------------------------------------- */
void setButtonState(int button, bool state) {

    // record if this is an actual state change
    if ( state != pushButton[button].state ) {
        if ( state ) {
            pushButton[button].lastOn = time(NULL);
        } else {
            pushButton[button].lastOff = time(NULL);
        }
    }
    
    pushButton[button].state     = state;
    pushButton[button].timestamp = state ? time(NULL) : (time_t) 0;

    saveState (pushButton[button].name, pushButton[button].state);

    writeLog(LOG_NOTICE, "%s toggled to %s", pushButton[button].name, pushButton[button].state ? "ON" : "OFF" );
    
    // if a radio button is pressed, make sure all other are swithced off
    if ( pushButton[button].state && pushButton[button].radio ) {
        int idxRadio = 0;
        while ( pushButton[idxRadio].name ) {
            if ( idxRadio != button && pushButton[idxRadio].state && pushButton[idxRadio].radio ) {
                pushButton[idxRadio].state = false;
                pushButton[idxRadio].timestamp = (time_t) 0;
                pushButton[idxRadio].lastOff = time(NULL);
                writeLog(LOG_NOTICE, "%s forced OFF by %s", pushButton[idxRadio].name, pushButton[button].name );
                saveState (pushButton[idxRadio].name, pushButton[idxRadio].state);
            }
            idxRadio++;
        }
    }
    setIoStates();
}


/* ----------------------------------------------------------------------------------- *
 * publish last state changes
 * ----------------------------------------------------------------------------------- */
void dumpEventHistory( void ) {
    int index = 0;
    struct tm *ts;
    
    while ( pushButton[index].name ) {
        if (pushButton[index].lastOn) {
            ts = localtime ( &(pushButton[index].lastOn) );
            mqttPublish("/State/LastOn", "%s %04d-%02d-%02d %02d:%02d", pushButton[index].name, ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday, ts->tm_hour, ts->tm_min );
        }
        
        if (pushButton[index].lastOff) {
            ts = localtime ( &(pushButton[index].lastOff) );
            mqttPublish("/State/LastOff", "%s %04d-%02d-%02d %02d:%02d", pushButton[index].name, ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday, ts->tm_hour, ts->tm_min );
        }
        index++;
    }
}

/* ----------------------------------------------------------------------------------- *
 * publish button states over mqtt
 * ----------------------------------------------------------------------------------- */
void publishButtonState(void) {
    int index = 0;
    char mqttTopic[32];

    while ( pushButton[index].name ) {
        sprintf(mqttTopic, "/State/%s", pushButton[index].name);
        mqttPublish(mqttTopic, pushButton[index].state ? "ON" : "OFF" );
        index++;
    }
}

/* ----------------------------------------------------------------------------------- *
 * Process MQTT commands
 * ----------------------------------------------------------------------------------- */
bool mqttMatch( const char* command, const char* topic ) {
    char pattern[128];
    bool match;
    
    sprintf(pattern, "/%s/Command/%s", mqttPrefix, command);
    mosquitto_topic_matches_sub(pattern, topic, &match);
    
    return match;
}

bool mqttParseEvent(char* payload, char *valve, bool *state, int *hour, int *minute ) {
    int  timeInd = 0;
    bool success = true;
    
    if ( strlen(payload) >= 15 ) {
        *valve = payload[6];
        if ( *valve == 'A' || *valve == 'B' || *valve == 'C' || *valve == 'D' ) {
            if ( payload[9] == 'F' ) {         // OFF
                *state = false;
                timeInd=12;
            } else if ( payload[9] == 'N' ) {  // ON
                *state = true;
                timeInd=11;
            } else {
                writeLog(LOG_WARNING, "parseEvent: Invalid State");
                success = false;
            }
            if ( success ) {
                if ( payload[timeInd+2] == ':' ) {
                    payload[timeInd+2] = (char)0;
                    payload[timeInd+5] = (char)0;
                    *hour   = atoi(payload+timeInd);
                    *minute = atoi(payload+timeInd+3);
                } else {
                    writeLog(LOG_WARNING, "parseEvent: Invalid time format");
                    success = false;
                }
            }
        } else {
            writeLog(LOG_WARNING, "parseEvent: Invalid Valve (%c)", *valve);
            success = false;
        }
    } else {
        writeLog(LOG_WARNING, "MQTT command 'addEvent': Invalid payload length (%d)", strlen(payload));
        success = false;
    }
    
    return success;
}

void mqttCommandCB(char *payload, int payloadlen, char *topic, void *user_data) {
    int   index = 0;
    writeLog(LOG_INFO, "Reveived MQTT Command: %s -> %s", topic, payload);
    
    // status update requested?
    if ( mqttMatch("Refresh", topic) ) {
        writeLog(LOG_NOTICE, "MQTT command: Send State info" );
        publishButtonState();

    // dump schedule table to broker
    } else if ( mqttMatch("dumpScheduleTable", topic) ) {
        dumpScheduleTable();
        
    // enable disable logging over mqtt
    } else if ( mqttMatch("mqttLoging", topic) ) {
        if ( !strncmp(payload, "ON", 2) ) {
            switchMQTTlog(true);
            writeLog(LOG_NOTICE, "Enabled logging to MQTT" );
        } else if ( !strncmp(payload, "OFF", 2) ) {
            writeLog(LOG_NOTICE, "Disabled logging to MQTT" );
            switchMQTTlog(false);
        }
    
    // publish last state changes
    } else if ( mqttMatch("getEventHistory", topic) ) {
        dumpEventHistory();

    // set valve timeout value
    } else if ( mqttMatch("setValveTimeout", topic) ) {
        int timeout = atoi ( payload );
        if ( timeout > 0 ) {
            valveMax = timeout;
            saveInt("valveMax", valveMax);
            mqttPublish("/State/valveTimeout", "%d", valveMax);
        }
        
    // publish valve timeout value
    } else if ( mqttMatch("getValveTimeout", topic) ) {
        mqttPublish("/State/valveTimeout", "%d", valveMax);
        
    // add event to schedule table
    } else if ( mqttMatch("addEvent", topic) ) {
        char valve;
        bool state;
        int  hour, minute;

        if ( mqttParseEvent(payload, &valve, &state, &hour, &minute) ) {
            addScheduleTableEvent(valve, state, hour, minute);
            if ( use_cache ) {
                saveScheduleTable(CACHE_FILE);
            }
        }

    // remove event from schedule table
    } else if ( mqttMatch("removeEvent", topic) ) {
        char valve;
        bool state;
        int  hour, minute;
        
        if ( mqttParseEvent(payload, &valve, &state, &hour, &minute) ) {
            removeScheduleTableEvent(valve, state, hour, minute);
            if ( use_cache ) {
                saveScheduleTable(CACHE_FILE);
            }
        }
                
    // enable/disable log messages over MQTT
    } else if ( mqttMatch("mqttLogging", topic) ) {
        if ( !strncmp(payload, "ON", 2) || !strncmp(payload, "1", 1) ) {
            char message[32];
            switchMQTTlog( true );
            sprintf(message, "%d (%s)", getLogLevel(), logLevelText[getLogLevel()] );
            mqttPublish("/State/LogLevel", message);
        } else if ( !strncmp(payload, "OFF", 2) || !strncmp(payload,"0", 1) ) {
            switchMQTTlog( false );
        }
        
    // set log level
    } else if ( mqttMatch("setLogLevel", topic) ) {
        for ( int logLevel = 0; logLevel <= MAX_LOG_LEVEL; logLevel++ ) {
            if ( !strncmp(payload, logLevelText[logLevel], strlen(logLevelText[logLevel]))) {
                setLogLevel(logLevel);
            }
        }
    // get log level
    } else if ( mqttMatch("getLogLevel", topic) ) {
        char message[32];
        sprintf(message, "%d (%s)", getLogLevel(), logLevelText[getLogLevel()] );
        mqttPublish("/State/LogLevel", message);

    // print log message cache
    } else if ( mqttMatch("printLog", topic) ) {
        printLog();
        
    // Exit excution
    } else if ( mqttMatch("exit", topic) ) {
        writeLog(LOG_WARNING, "Stopping execution on MQTT request" );
        for ( int index = 0; index < 4; index ++ ) { // turn off all valves
            setButtonState(index, false);
        }
        shutdown_daemon(EXIT_SUCCESS);
        
    // check for match with button
    } else {
        while ( pushButton[index].name ) {
            if ( mqttMatch( pushButton[index].name, topic) ) {
                writeLog(LOG_INFO, "Matched button: %s", pushButton[index].name );
                
                if ( !strncmp(payload, "ON", 2) || !strncmp(payload, "1", 1) ) {
                    writeLog(LOG_NOTICE, "MQTT command: Switch %s ON", pushButton[index].name );
                    setButtonState(index, true);
                    publishButtonState();
                } else if ( !strncmp(payload, "OFF", 2) || !strncmp(payload,"0", 1) ) {
                    writeLog(LOG_NOTICE, "MQTT command: Switch %s OFF", pushButton[index].name );
                    setButtonState(index, false);
                    publishButtonState();
                } else {
                    writeLog(LOG_WARNING, "Uknown command for %s: %s", pushButton[index].name, payload);
                }
            }
            index++;
        }
    }
}

/* ----------------------------------------------------------------------------------- *
 * Endless Loop
 * ----------------------------------------------------------------------------------- */
void mainLoop(void) {
    time_t lastTime      = 0;
    time_t lastBroadcast = 0;
    
    for ( ;; ) {
        time_t now = time(NULL);
        if ( now >= (lastTime+10) ) {    // once every ten seconds
            lastTime = now;
            if (pushButton[INDEX_TIMER].state ) {
                processScheduleTable();  // trigger events as scheduled
            }
            setIoStates();               // refresh output IO port states
            valveTimeOut(valveMax);      // make sure active valves time-out
        }
        
        if ( now >= (lastBroadcast+300) ) {  // publish button states every 5 minutes
            lastBroadcast = now;
            publishButtonState();
        }
        
        pollButtons();                   // process changes in button state
        delay(50);                       // avoid busy loop
    }
}

/* ----------------------------------------------------------------------------------- *
 * Main
 * ----------------------------------------------------------------------------------- */
int main( int argc, char *argv[] ) {
    char *scheduleTableFile = NULL;
    mqttPrefix = "YardControl";
    
    // Process command line options
    for (int i=0; i<argc; i++) {
        if (!strcmp(argv[i], "-d")) {          // '-d' increase debug level
            debug++;
        }
        
        if (!strcmp(argv[i], "-f")) {          // '-f' force forground mode
            foreground=true;
        }
        
        if (!strcmp(argv[i], "-s")) {          // '-s' specify schedule file name
            scheduleTableFile = strdup(argv[++i]);
        }

        if (!strcmp(argv[i], "-c")) {          // '-c' clear schedule table cache
            use_cache = false;
        }
        
        if (!strcmp(argv[i], "-p")) {          // '-p' prefix to be used for MQTT topics
            mqttPrefix = strdup(argv[++i]);
        }
    }

    // initialize logging channel
    initLog(!foreground);
    setLogLevel(LOG_NOTICE+debug);
    
    if (!foreground) {
        // run in background
        daemonize(PID_FILE);
    } else {
        writeLog(LOG_NOTICE, "Running in foreground");
    }
    
    // set correct prefix for incoming messages
    char topic[128];
    sprintf(topic, "/%s/Command/#", mqttPrefix );
    subscriptions[0].topic = strdup(topic);
    
    if (mqttInit(mqttPrefix, MQTT_BROKER_HOST, MQTT_BROKER_PORT, 60, subscriptions) ) {
        // load event schedule
        initScheduleTable();
        loadScheduleTable(scheduleTableFile);
        if ( use_cache ) {
            loadScheduleTable(CACHE_FILE);
        }
        
        // restore timer setting
        setButtonState(INDEX_TIMER, readState(pushButton[INDEX_TIMER].name));
        
        // restore timeout value
        readInt( "valveMax", &valveMax);
        
        // Initialize IO ports
        setupIO();

        // broadcast initial state
        publishButtonState();
        
        // get busy
        mainLoop();
    } else {
        writeLog(LOG_ERR, "Error: Could not connect to MQTT broker at localhost:1883");
    }
    return 0;
}
