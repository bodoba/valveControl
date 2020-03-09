/* *********************************************************************************** */
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

#ifndef mqttGateway_h
#define mqttGateway_h
#include <stdio.h>

/* ----------------------------------------------------------------------------------- *
 * Define to turn on MQTT debug messages
 * ----------------------------------------------------------------------------------- */
//#define MQTT_DEBUG

/* ----------------------------------------------------------------------------------- *
 * handler for incoming MQTT messages
 * ----------------------------------------------------------------------------------- */
/*
 * Callback prototype:
 *   void switchValveCB(char *payload, int payloadlen, char *topic, void *user_data);
 */
typedef struct mqttIncoming_t {
    const char* topic;                           // MQTT topic to subscribe to
    void  (*handler)(char*, int, char*, void*);  // callback function
    void  *user_data;                            // user defined argument to callback
} mqttIncoming_t;

/* ----------------------------------------------------------------------------------- *
 * Exported functions
 * ----------------------------------------------------------------------------------- */
bool mqttInit(const char* broker, int port, int keepalive, mqttIncoming_t *subscriptions);
void mqttEnd(void );
bool mqttPublish(const char *topic, const char* format, ... );

#endif /* mqttGateway_h */
