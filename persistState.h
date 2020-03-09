/* *********************************************************************************** */
/*  Copyright (c) 2019 by Bodo Bauer <bb@bb-zone.com>                                  */
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

#ifndef persistState_h
#define persistState_h

/* ----------------------------------------------------------------------------------- *
 * Default Settings
 * ----------------------------------------------------------------------------------- */
#define STATE_DIR    "/var/run/valveControl"            // store state files here

/* ----------------------------------------------------------------------------------- *
 * Some globals we can't do without
 * ----------------------------------------------------------------------------------- */
extern char *stateDir;                                // directory for state files

/* ----------------------------------------------------------------------------------- *
 * Prototypes
 * ----------------------------------------------------------------------------------- */
void saveState ( const char *name, bool value );      // safe state of boolean value
bool readState ( const char *name );                  // read named state

#endif /* persistState_h */
