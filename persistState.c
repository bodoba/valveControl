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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "logging.h"
#include "persistState.h"

/* ----------------------------------------------------------------------------------- *
 * Some globals we can't do without
 * ----------------------------------------------------------------------------------- */
char *stateDir      = STATE_DIR;              // directory for state files


/* ----------------------------------------------------------------------------------- *
 * Save integer value in named file
 * ----------------------------------------------------------------------------------- */
void saveInt ( const char *name, int value ) {
    char *fname = malloc( sizeof(char) * ( strlen(stateDir)+strlen(name) + 2 ) );
    sprintf( fname, "%s/%s", stateDir, name );
    FILE *fd = fopen(fname, "w" );
    fprintf(fd, "%08d \n", value);
    fclose ( fd );
    free(fname);
    writeLog(LOG_DEBUG, "saveInt( %s, %d )", name, value );
}

/* ----------------------------------------------------------------------------------- *
 * Save integer value in named file
 * ----------------------------------------------------------------------------------- */
bool readInt ( const char *name, int *value ) {
    bool retval = false;
    char *fname = malloc( sizeof(char) * ( strlen(stateDir)+strlen(name) + 2 ) );
    sprintf( fname, "%s/%s", stateDir, name );
    FILE *fd = fopen(fname, "r" );
    if ( fd ) {
        fscanf( fd, "%d", value );
        fclose (fd);
        writeLog(LOG_DEBUG, "readInt( %s, %d )", name, *value );
    }
    free(fname);
    return retval;
}
/* ----------------------------------------------------------------------------------- *
 * Save state by creating/removing a file in the state file directory
 * ----------------------------------------------------------------------------------- */
void saveState (const char *name, bool state) {
    if (readState(name) != state) {
        char *fname = malloc( sizeof(char) * ( strlen(stateDir)+strlen(name) + 2 ) );
        sprintf( fname, "%s/%s", stateDir, name );
        if (state) {
            int fd = open(fname, O_CREAT | O_WRONLY, S_IRWXU );
            close ( fd );
        } else {
            unlink(fname);
        }
        free(fname);
    }
    writeLog(LOG_DEBUG, "saveState( %s, %s )", name, state ? "TRUE" : "FALSE" );
}

/* ----------------------------------------------------------------------------------- *
 * Read state by checking if a file of the given name exists
 * ----------------------------------------------------------------------------------- */
bool readState (const char *name) {
    bool state = false;
    char *fname = malloc(sizeof(char) * ( strlen(stateDir)+strlen(name) + 2 ) );
    struct stat buf;
    sprintf( fname, "%s/%s", stateDir, name );
    if (!stat(fname, &buf)) {
        state = true;
    }
    free (fname);
    writeLog(LOG_DEBUG, "readState( %s ) -> %s", name, state ? "TRUE" : "FALSE" );
    return state;
}
