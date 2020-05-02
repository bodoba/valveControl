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
#ifndef logging_h
#define logging_h

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <syslog.h>

/*
  log levels are taken from syslog.h
  LOG_EMERG    A panic condition was reported to all processes.
  LOG_ALERT    A condition that should be corrected immediately.
  LOG_CRIT     A critical condition.
  LOG_ERR      An error message.
  LOG_WARNING  A warning message.
  LOG_NOTICE   A condition requiring special handling.
  LOG_INFO     A general information message.
  LOG_DEBUG    A message useful for debugging programs.
 */

extern const char * logLevelText[];

// Numer of log entries to cache (1k per entry)
#define LOG_CACHE_SIZE 100

/* ----------------------------------------------------------------------------------- *
 * Prototypes
 * ----------------------------------------------------------------------------------- */
int setLogLevel( int level );
int getLogLevel( void );

void switchMQTTlog( bool on );

void initLog( bool useSyslog );
void writeLog( int logLevel, const char* format, ... );

#endif /* logging_h */
