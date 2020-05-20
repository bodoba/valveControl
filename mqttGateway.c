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
#include <mosquitto.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "mqttGateway.h"
#include "logging.h"

/* ----------------------------------------------------------------------------------- *
 * Handle to broker
 * ----------------------------------------------------------------------------------- */
static struct mosquitto *mosq = NULL;

/* ----------------------------------------------------------------------------------- *
 * List of topics to subscribe to along with handlers to call on reception
 * ----------------------------------------------------------------------------------- */
static        mqttIncoming_t *subscriptionList = NULL;
static        const char* mqttPrefix;
/* ----------------------------------------------------------------------------------- *
 * Local prototypes
 * ----------------------------------------------------------------------------------- */
static void mqttLog(struct mosquitto *mosq, void *user_data, int logLevel, const char *logMessage);

/* ----------------------------------------------------------------------------------- *
 * Proxy to redirect mosquitto log messages to writeLog
 * ----------------------------------------------------------------------------------- */
void mqttLog(struct mosquitto *mosq, void *user_data, int logLevel, const char *logMessage) {
#ifdef MQTT_DEBUG
    writeLog(LOG_INFO, logMessage);
#endif
}

/* ----------------------------------------------------------------------------------- *
 * Dispatch incoming messages
 * ----------------------------------------------------------------------------------- */
void dispatchMessage(struct mosquitto *mos, void *userData, const struct mosquitto_message *message) {
    // identify callback functiion by matching topic
    int idx = 0;
    bool match;
    while (subscriptionList[idx].topic) {
        mosquitto_topic_matches_sub(subscriptionList[idx].topic, message->topic, &match);
        if(match) {
            (subscriptionList[idx].handler)(message->payload,
                                            message->payloadlen,
                                            message->topic,
                                            subscriptionList[idx].user_data);
            break;
        }
        idx++;
    }
}

/* ----------------------------------------------------------------------------------- *
 * Connect to MQTT broker
 * ----------------------------------------------------------------------------------- */
bool mqttInit( const char* prefix, const char* broker, int port, int keepalive, mqttIncoming_t *subscriptions) {
    bool success = true;
    int err;
    mqttPrefix = prefix;
    
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if(mosq){
        err = mosquitto_connect(mosq, broker, port, keepalive);
        if( err != MOSQ_ERR_SUCCESS ) {
            writeLog(LOG_ERR, "Error: mosquitto_connect [%s]\n", mosquitto_strerror(err));
            success = false;
        }
    } else {
        writeLog(LOG_ERR, "Error: Out of memory.\n");
        success = false;
    }
    
    err = mosquitto_loop_start(mosq);
    if( err != MOSQ_ERR_SUCCESS ) {
        writeLog(LOG_ERR, "Error: mosquitto_connect [%s]\n", mosquitto_strerror(err));
        success = false;
    } else {
        mosquitto_log_callback_set(mosq, &mqttLog);

        mosquitto_message_callback_set(mosq, &dispatchMessage);
        if ( subscriptions ) {
            subscriptionList = subscriptions;
            int idx = 0;
            while (subscriptionList[idx].topic) {
                writeLog(LOG_INFO, "Subscribe to MQTT topic: %s", subscriptionList[idx].topic);
                mosquitto_subscribe( mosq, NULL, subscriptionList[idx].topic, 0);
                idx++;
            }
        }
    }
    return success;
}

/* ----------------------------------------------------------------------------------- *
 * End MQTT broker connection
 * ----------------------------------------------------------------------------------- */
void mqttEnd( void ) {
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    mosq = NULL;
}

/* ----------------------------------------------------------------------------------- *
 * Publish MQTT message
 * ----------------------------------------------------------------------------------- */
bool mqttPublish(const char *topic, const char* format, ... ) {
    va_list valist;
    int err;
    char message[512];
    char pTopic[512];
    bool success = true;

    sprintf(pTopic, "/%s%s", mqttPrefix, topic);
    
    va_start(valist, format);
    vsprintf(message, format, valist);
    
    if ( mosq ) {
        err = mosquitto_publish( mosq, NULL, pTopic, strlen(message), message, 0, false);
        if ( err != MOSQ_ERR_SUCCESS) {
            writeLog(LOG_ERR, "Error: mosquitto_publish failed [%s]\n", mosquitto_strerror(err));
            success = false;
        }
    } else {
        writeLog(LOG_ERR, "Error: mosq == NULL, Init failed?\n");
        success = false;
    }
    return success;
}
