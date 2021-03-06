/* *********************************************************************************** */
/*                                                                                     */
/*  Copyright (c) 2018 by Bodo Bauer <bb@bb-zone.com>                                  */
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
#include <syslog.h>
#include <string.h>

#include "logging.h"
#include "mqttGateway.h"

/* ----------------------------------------------------------------------------------- *
 * local data
 * ----------------------------------------------------------------------------------- */
static int  logLevel     = LOG_ERR;
static bool useSyslog    = true;
static bool useMQTTlog   = false;
static void addToCache( const char* logmessage );

/* ----------------------------------------------------------------------------------- *
 * ring buffer to cache log entries
 * ----------------------------------------------------------------------------------- */
static char ringBuffer[LOG_CACHE_SIZE][MAX_LOG_MESSAGE_SIZE];
static int  ringBufferNext=0;

/* ----------------------------------------------------------------------------------- *
 * exported data
 * ----------------------------------------------------------------------------------- */
const char * logLevelText[] = {
    "EMERGENCY",
    "ALERT",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG"
};

/* ----------------------------------------------------------------------------------- *
 * init logging
 * ----------------------------------------------------------------------------------- */
void initLog( bool syslog ) {
    useSyslog = syslog;
    
    if ( useSyslog) {
        openlog(NULL, LOG_PID, LOG_USER);                 // use syslog to create a trace
    }

    // initialize ringbuffer
    for ( int i=0; i<LOG_CACHE_SIZE; i++) {
        ringBuffer[i][0] = (char)0;
    }
}

/* ----------------------------------------------------------------------------------- *
 * set Loglevel
 * ----------------------------------------------------------------------------------- */
int setLogLevel( int level ) {
    logLevel = level;
    if (logLevel > LOG_DEBUG ) {
        logLevel = LOG_DEBUG;
    }
    if (logLevel < LOG_EMERG ) {
        logLevel = LOG_EMERG;
    }
    writeLog(LOG_INFO, "Set log level to %s", logLevelText[level]);
    return logLevel;
}
    
/* ----------------------------------------------------------------------------------- *
 * enable/disable logging to MQTT
 * ----------------------------------------------------------------------------------- */
void switchMQTTlog( bool on ) {
    useMQTTlog = on;
    writeLog(LOG_INFO, "%sable logging to MQTT", on ? "En" : "Dis");
}

/* ----------------------------------------------------------------------------------- *
 * add log entry to cache
 * ----------------------------------------------------------------------------------- */
static void addToCache(const char* logmessage) {
    strncpy(ringBuffer[ringBufferNext], logmessage, MAX_LOG_MESSAGE_SIZE);
    ringBufferNext++;
    
    if ( ringBufferNext >= LOG_CACHE_SIZE) {
        ringBufferNext = 0;
    }
}

/* ----------------------------------------------------------------------------------- *
 * print cached log entries to MQTT
 * ----------------------------------------------------------------------------------- */
void printLog( void ) {
    int index, count=0;
    writeLog(LOG_INFO, "Print log cache to MQTT");

    if ( ringBufferNext == 0 ) {
        index = LOG_CACHE_SIZE-1;
    } else {
        index = ringBufferNext-1;
    }
    
    while ( ringBuffer[index][0] && ++count < LOG_CACHE_SIZE ) {
        mqttPublish("/Log", ringBuffer[index]);
        if ( --index <= 0 ) {
          index = LOG_CACHE_SIZE-1;
        }
    }
}

/* ----------------------------------------------------------------------------------- *
 * return Loglevel
 * ----------------------------------------------------------------------------------- */
int getLogLevel( void ) {
    return logLevel;
}

/* ----------------------------------------------------------------------------------- *
 * write log entry
 * ----------------------------------------------------------------------------------- */
void writeLog( int level, const char* format, ...) {
    static unsigned long sequenceCounter = 0;
    va_list valist;
    char msg[MAX_LOG_MESSAGE_SIZE];
    char fmt[MAX_LOG_MESSAGE_SIZE];
    
    if( level <= logLevel ) {
        time_t now = time(NULL);
        struct tm *timestamp = localtime(&now);
        va_start(valist, format);

        sprintf(fmt, "%06lu %04d-%02d-%02d %02d:%02d:%02d <%s> %s", sequenceCounter,
                timestamp->tm_year+1900, timestamp->tm_mon+1, timestamp->tm_mday,
                timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec,
                logLevelText[level] , format);
        vsprintf( msg, fmt, valist );
        
        if ( useSyslog ) {
            sprintf(fmt, "<%s> %s\n", logLevelText[level], format);
            vsyslog( level, fmt, valist );
        } else {
            sprintf(fmt, "%s\n", fmt);
            vprintf( fmt, valist );
        }
    
        if ( useMQTTlog ) {
            mqttPublish( "/Log", msg);
        }
        addToCache( msg );
        sequenceCounter++;
    }
}
