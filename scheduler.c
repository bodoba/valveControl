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
#include <string.h>
#include "scheduler.h"
#include "valveControl.h"
#include "logging.h"
#include "mqttGateway.h"

/* ----------------------------------------------------------------------------------- *
 * Table with scheduled events
 * ----------------------------------------------------------------------------------- */
scheduleEvent_t scheduleTable[SCHEDULE_TABLE_ENTRIES];

/* ----------------------------------------------------------------------------------- *
 * Initialize schedule table
 * ----------------------------------------------------------------------------------- */
void initScheduleTable(void) {
    writeLog(LOG_INFO, "Initialize schedule table" );

    for ( int index=0; index<SCHEDULE_TABLE_ENTRIES; index++ ) {
        scheduleTable[index].valve       = (char)0;  // 0 marks free entry
        scheduleTable[index].state       = false;
        scheduleTable[index].hour        = 0;
        scheduleTable[index].minute      = 0;
        scheduleTable[index].buttonIndex = 0;
    }
}

/* ----------------------------------------------------------------------------------- *
 * Load schedule table from file (replace exisitng schedule table)
 * ----------------------------------------------------------------------------------- */
bool loadScheduleTable(const char *scheduleTableFile) {
    bool success = false;
    FILE *fp = NULL;
    if (scheduleTableFile) {
        fp = fopen(scheduleTableFile, "rb");
        if (fp) {
            char  *line=NULL, *cursor, valve;
            bool state;
            int hour, minute, lineNo=0;
            size_t n, length = getline(&line, &n, fp);
                        
            writeLog(LOG_NOTICE, "Loading Scheulde from %s", scheduleTableFile );
            
            while ( length != -1) {
                if ( length > 1 ) {                                      // skip empty lines
                    cursor = line;
                    if (line[length-1] == '\n') line[length-1] = '\0';   // remove trailing newline
                    while (*cursor == ' ' || *cursor == '\t') cursor++;  // remove leading whitespace
                    if ( *cursor != '#') {                               // skip '#' comments
                        writeLog(LOG_DEBUG, "[%s:%04d] %s", scheduleTableFile, lineNo, cursor );

                        // which valve?
                        switch (*cursor) {
                            case 'A':
                                valve = 'A';
                                cursor++;
                                break;
                            case 'B':
                                valve = 'B';
                                cursor++;
                                break;
                            case 'C':
                                valve = 'C';
                                cursor++;
                                break;
                            case 'D':
                                valve = 'D';
                                cursor++;
                                break;
                            default:
                                valve = (char)0;
                                break;
                        }
                        
                        if (valve) {
                            bool state_ok = true;
                            while (*cursor == ' ' || *cursor == '\t') cursor++;  // skip whitespace
                            
                            // which state?
                            if (!strncmp(cursor, "ON", 2)) {
                                state = true;
                                cursor += 2;
                            } else if (!strncmp(cursor, "OFF", 3)) {
                                state = false;
                                cursor += 3;
                            } else {
                                state_ok = false;
                                writeLog(LOG_WARNING, "[%s:%04d] Could not identify state (valid values: ON, OFF)", scheduleTableFile, lineNo );
                            }
                            
                            if ( state_ok ) {
                                while (*cursor == ' ' || *cursor == '\t') cursor++;  // skip whitespace
                                if ( cursor[2] == ':' ) {
                                    // which time?
                                    cursor[2] = (char)0;
                                    hour = atoi(cursor);
                                    cursor[5] = (char)0;
                                    minute = atoi(cursor+3);
                                    
                                    if ( hour >= 0 && hour < 24 && minute >=0 && minute < 60 ) {
                                        addScheduleTableEvent(valve, state, hour, minute);
                                    } else {
                                        writeLog(LOG_WARNING, "[%s:%04d] invalid time (valid hh:mm)", scheduleTableFile, lineNo );
                                    }
                                } else {
                                    writeLog(LOG_WARNING, "[%s:%04d] invalid time (valid hh:mm)", scheduleTableFile, lineNo );
                                }
                            }
                        } else {
                            writeLog(LOG_WARNING, "[%s:%04d] Could not identify valve '%c' (valid values: A, B, C, D)", scheduleTableFile, lineNo, *cursor );
                        }
                    }
                }
                free(line);
                n=0;
                length = getline(&line, &n, fp);
                lineNo++;
            }
            fclose(fp);
            success = true;
        } else {
            writeLog(LOG_WARNING, "Could not open %s", scheduleTableFile );
        }
    }
    return success;
}

/* ----------------------------------------------------------------------------------- *
 * Dump schedule table to mqtt broker
 * ----------------------------------------------------------------------------------- */
void dumpScheduleTable(void) {
    for ( int index=0; index<SCHEDULE_TABLE_ENTRIES; index++) {
        if ( scheduleTable[index].valve ) {
            mqttPublish("/YardControl/ScheduleTable/Entry", "Valve_%c %s %02d:%02d",
                scheduleTable[index].valve,
                scheduleTable[index].state ? "ON" : "OFF",
                scheduleTable[index].hour,
                scheduleTable[index].minute );
        }
    }
}

/* ----------------------------------------------------------------------------------- *
 * Save schedule table to file
 * ----------------------------------------------------------------------------------- */
bool saveScheduleTable(const char *fname) {
    bool success = false;
    
    FILE *stream = fopen(fname, "w+");
    if ( stream ) {
        fprintf ( stream, "#\n# Valve Control Schedule Table\n#\n");
                
        for ( int index=0; index<SCHEDULE_TABLE_ENTRIES; index++) {
            if ( scheduleTable[index].valve ) {
                fprintf(stream, "%c %s %02d:%02d\n",
                        scheduleTable[index].valve,
                        scheduleTable[index].state ? "ON" : "OFF",
                        scheduleTable[index].hour,
                        scheduleTable[index].minute );
            }
        }
        fclose(stream);
    } else {
        writeLog(LOG_WARNING, "Can't open file <%s> for writing", fname);
    }
    return success;
}

/* ----------------------------------------------------------------------------------- *
 * Match button name with valve and return index
 * ----------------------------------------------------------------------------------- */
int getButtonIndex( char valve ) {
    int buttonIndex = -1;
    char buttonName[] = "Valve_X";
    buttonName[6] = valve;
    int index = 0;
    
    while ( pushButton[index].name ) {
        if ( !strncmp(buttonName, pushButton[index].name, 7) ) {
            buttonIndex = index;
            break;
        }
        index++;
    }
    
    return buttonIndex;
}

/* ----------------------------------------------------------------------------------- *
 * Remove event from schedule table
 * ----------------------------------------------------------------------------------- */
bool removeScheduleTableEvent(char valve, bool state, int hour, int minute) {
    bool success = false;
    int index=0;
    // find event entry
    for ( index=0; index < SCHEDULE_TABLE_ENTRIES; index++ ) {
        if ( scheduleTable[index].valve == valve ) {
            if (   scheduleTable[index].state  == state
                && scheduleTable[index].hour   == hour
                && scheduleTable[index].minute == minute ) {
                success = true;
                break;
            }
        }
    }

    if (success) {
        writeLog(LOG_NOTICE, "Remove schedule table event #%03d: Valve %c %s at %02d:%02d",
                 index, valve, state ? "ON ":"OFF", hour, minute);
        scheduleTable[index].valve = (char)0;
    } else {
        writeLog(LOG_WARNING, "removeEvent: No matching event not found in schedule table", valve);
    }
    
    return success;
}

/* ----------------------------------------------------------------------------------- *
 * process shedule table
 * ----------------------------------------------------------------------------------- */
void processScheduleTable(void) {
    time_t t = time(NULL);
    struct tm *now = localtime(&t);
    int hour   = now->tm_hour;
    int minute = now->tm_min;
    
    for ( int index=0; index<SCHEDULE_TABLE_ENTRIES; index++) {
        if ( scheduleTable[index].valve ) {

            if (  scheduleTable[index].hour   == hour
                && scheduleTable[index].minute == minute ) {
                writeLog(LOG_INFO, "Schedule Table Trigger at %02d:%02d: %c %s",
                         hour, minute, scheduleTable[index].valve,
                         scheduleTable[index].state ? "ON" : "OFF" );
                setButtonState( scheduleTable[index].buttonIndex, scheduleTable[index].state);
            }
        }
    }
}

/* ----------------------------------------------------------------------------------- *
 * Add event to schedule table
 * ----------------------------------------------------------------------------------- */
bool addScheduleTableEvent(char valve, bool state, int hour, int minute) {
    int success = true;
    int index = 0;
    
    // Input validation
    if ( valve != 'A' && valve != 'B' && valve != 'C' && valve != 'D') {
        writeLog(LOG_WARNING, "addEvent: Invalid valve (%c)", valve);
        success = false;
    }
    if ( hour < 0 || hour > 23 || minute < 0 || minute > 59 ) {
        writeLog(LOG_WARNING, "addEvent: Invalid time (%02d:%02d)", hour, minute);
        success = false;
    }
    
    // check for duplicate
    for ( index = 0; index < SCHEDULE_TABLE_ENTRIES; index++ ) {
        if ( scheduleTable[index].valve != (char)0 ) {
            if (   scheduleTable[index].valve  == valve
                && scheduleTable[index].state  == state
                && scheduleTable[index].hour   == hour
                && scheduleTable[index].minute == minute ) {
                writeLog(LOG_WARNING, "addEvent: Ignoring duplicate" );
                success = false;
                break;
            }
        }
    }

    if ( success ) {
        // skip to next free entry
        index = 0;
        while ( scheduleTable[index].valve != (char)0 && index < SCHEDULE_TABLE_ENTRIES ) index++;
        if ( index < SCHEDULE_TABLE_ENTRIES ) {
            int buttonIndex = getButtonIndex ( valve );
            if ( buttonIndex >= 0 ) {
                scheduleTable[index].valve       = valve;
                scheduleTable[index].state       = state;
                scheduleTable[index].hour        = hour;
                scheduleTable[index].minute      = minute;
                scheduleTable[index].buttonIndex = buttonIndex;
                
                writeLog(LOG_NOTICE, "Add schedule table event #%03d: Valve %c %s at %02d:%02d (button #%02d)",
                         index, valve, state?"ON ":"OFF", hour, minute, buttonIndex );
            } else {
                writeLog(LOG_WARNING, "Could not add new event, did not find button for valve %c", valve );
                success = false;
            }
        } else {
            writeLog(LOG_WARNING, "Could not add new event, no free slot in event table" );
            success = false;
        }
    }
    
    return success;
}
