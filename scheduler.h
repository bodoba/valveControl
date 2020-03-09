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

#ifndef scheduler_h
#define scheduler_h

#include <stdbool.h>

typedef struct scheduleEvent_t {
    char valve;
    bool state;
    int  hour;
    int  minute;
    int  buttonIndex;
} scheduleEvent_t;

// the maximum size of the schedule table
#define SCHEDULE_TABLE_ENTRIES 100

extern scheduleEvent_t scheduleTable[];

void initScheduleTable(void);
void dumpScheduleTable(void);
bool loadScheduleTable(const char *scheduleTableFile);
bool saveScheduleTable(const char *scheduleTableFile);
bool addScheduleTableEvent(char valve, bool state, int hour, int minute);
bool removeScheduleTableEvent(char valve, bool state, int hour, int minute);
void processScheduleTable(void);

#endif /* scheduler_h */
