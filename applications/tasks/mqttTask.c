/*
 * Copyright 2022 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *
 * MQTT Task to connect to the broker and to keep the connection
 *
 */

#include "common.h"
#include "taskControl.h"
#include "mqttTask.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define MQTT_TASK_STACK_SIZE 1024
#define MQTT_TASK_PRIORITY 5
#define MQTT_QUEUE_STACK_SIZE 1024
#define MQTT_QUEUE_PRIORITY 5
#define MQTT_QUEUE_SIZE 10

#define COPYTO(msg, x) ((msg.x = uStrDup(x))==NULL) ? true : failed

#define MAX_TOPIC_SIZE 100
#define MAX_MESSAGE_SIZE (12 * 1024 + 1)    // set this to 12KB as this
                                            // is the same buffer size
                                            // in the modules plus 1
                                            // for the null

#define MAX_TOPIC_CALLBACKS 50

#define TEMP_TOPIC_NAME_SIZE 150

/* ----------------------------------------------------------------
 * COMMON TASK VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig = NULL;

/* ----------------------------------------------------------------
 * TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef struct TOPIC_CALLBACK {
    char *topicName;
    uMqttQos_t qos;

    int32_t numCallbacks;
    callbackCommand_t *callbacks;
} topicCallback_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static uMqttClientContext_t *pContext = NULL;
static int32_t messagesToRead = 0;
static char topicString[MAX_TOPIC_SIZE];
static char *downlinkMessage;

static int32_t topicCallbackCount = 0;
static topicCallback_t *topicCallbackRegister[MAX_TOPIC_CALLBACKS];

static char tempTopicName[TEMP_TOPIC_NAME_SIZE];
/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Send an MQTT Message - remember to FREE the msg memory!!!!
/// @param msg The message to send.
static void mqttSendMessage(sendMQTTMsg_t msg)
{
    int32_t errorCode = 0;

    if (pContext != NULL && uMqttClientIsConnected(pContext)) {
        gAppStatus = MQTT_CONNECTED;
        errorCode = uMqttClientPublish(pContext, msg.pTopicName, msg.pMessage,
                                                    strlen(msg.pMessage),
                                                    msg.QoS,
                                                    msg.retain);
    } else {
        errorCode = U_ERROR_COMMON_NOT_INITIALISED;
        writeDebug("MQTT Client is not online, not publishing message");
    }

    uPortFree(msg.pTopicName);
    uPortFree(msg.pMessage);

    if (errorCode == 0)
        writeDebug("Published MQTT message");
    else {
        if (errorCode != U_ERROR_COMMON_NOT_INITIALISED)
            writeWarn("Failed to publish MQTT message: %d", errorCode);
    }
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    mqttMsg_t *qMsg = (mqttMsg_t *) pParam;

    switch(qMsg->msgType) {
        case SEND_MQTT_MESSAGE:
            mqttSendMessage(qMsg->msg.message);
            break;

        default:
            writeLog("Unknown message type: %d", qMsg->msgType);
            break;
    }
}

static void disconnectCallback(int32_t errorCode, void *param)
{
    if (errorCode != 0) {
        gAppStatus = MQTT_DISCONNECTED;
        writeError("MQTT Disconnect callback with Error Code: %d", errorCode);
    }
}

static void downlinkMessageCallback(int32_t msgCount, void *param)
{
    printDebug("Got a downlink MQTT message notification: %d", msgCount);
    messagesToRead = msgCount;
}

static int32_t connectBroker(void)
{
    gAppStatus = MQTT_CONNECTING;
    writeLog("Connecting to MQTT Broker...");
    uMqttClientConnection_t connection = U_MQTT_CLIENT_CONNECTION_DEFAULT;
    connection.pBrokerNameStr = getConfig("MQTT_BROKER_NAME");
    connection.pUserNameStr = getConfig("MQTT_USERNAME");
    connection.pPasswordStr = getConfig("MQTT_PASSWORD");
    connection.inactivityTimeoutSeconds = 0; // zero = no timeout
    connection.keepAlive = false;

    int32_t errorCode = uMqttClientConnect(pContext, &connection);
    if (errorCode != 0) {
        writeError("Failed to connect to the MQTT Broker: %d", errorCode);
        return errorCode;
    }

    gAppStatus = MQTT_CONNECTED;
    errorCode = uMqttClientSetDisconnectCallback(pContext, disconnectCallback, NULL);
    if (errorCode != 0) {
        writeError("Failed to set MQTT Disconnect callback: %d", errorCode);
        return errorCode;
    }

     errorCode = uMqttClientSetMessageCallback(pContext, downlinkMessageCallback, NULL);
    if (errorCode != 0) {
        writeError("Failed to set MQTT downlink message callback: %d", errorCode);
        return errorCode;
    }

    writeLog("Connected to MQTT Broker");
    return 0;
}

static int32_t disconnectBroker(void)
{
    writeLog("Disconnecting from MQTT broker...");
    int32_t errorCode = uMqttClientDisconnect(pContext);
    if (errorCode != 0) {
        writeError("Failed to disconnect from MQTT Broker: %d", errorCode);
    } else {
        writeLog("Disconnected from MQTT Broker");
    }

    return errorCode;
}

static bool isNotExiting(void)
{
    return !gExitApp && !exitTask;
}

/// @brief Flag to indicate we can continue to dwell and wait for an event
/// @return True if we can keep dwelling, false otherwise
static bool continueToDwell(void)
{
    return isNotExiting() && (messagesToRead == 0);
}

static void freeCallbacks(void)
{
    int32_t c = topicCallbackCount;

    // Take a copy of the callback count and reset
    // This is so that any callback that might occur
    // now is not found
    topicCallbackCount = 0;

    for(int i=0; i<c; i++) {
        uPortFree(topicCallbackRegister[i]->topicName);
        topicCallbackRegister[i]->topicName = NULL;

        uPortFree(topicCallbackRegister[i]);
        topicCallbackRegister[i] = NULL;
    }
}

/// @brief Read an MQTT message
/// @return the size of the message which has been read, or negative on error
static int32_t readMessage(void)
{
    if (downlinkMessage == NULL) {
        writeError("MQTT downlink message buffer NULL, can't read message!");
        return U_ERROR_COMMON_NO_MEMORY;
    }

    int32_t errorCode;
    size_t msgSize = MAX_MESSAGE_SIZE;
    uMqttQos_t QoS;
    printDebug("Reading MQTT Message...");
    errorCode = uMqttClientMessageRead(pContext, topicString, MAX_TOPIC_SIZE, downlinkMessage, &msgSize, &QoS);
    if (errorCode < 0) {
        writeError("Failed to read the MQTT Message: %d", errorCode);
        return errorCode;
    } else {
        printDebug("Read MQTT Message on topic: %s [%d bytes]", topicString, msgSize);
        downlinkMessage[msgSize] = 0x00;
    }

    return msgSize;
}

static int32_t runCommandCallback(callbackCommand_t *callbacks, int32_t numCallbacks, char *message, size_t msgSize)
{
    int32_t errorCode = U_ERROR_COMMON_INVALID_PARAMETER;
    commandParamsList_t *params = NULL;
    size_t count = getParams(message, &params);
    if (count == 0) {
        writeError("No command/param found in message: '%s'", message);
        goto cleanUp;
    }

    char *command = params->parameter;
    if (command == NULL) {
        writeError("Parsed MQTT command, but no command was set.");
        goto cleanUp;
    }

    for(int i=0; i<numCallbacks; i++) {
        if(strncmp(command, callbacks[i].command, msgSize) == 0) {
            errorCode = callbacks[i].callback(params);
            goto cleanUp;
        }
    }

    writeWarn("Didn't find command '%s' in callbacks", command);
    errorCode = U_ERROR_COMMON_NOT_FOUND;

cleanUp:
    freeParams(params);
    return errorCode;
}

/// @brief Find the callback for the topic we have just received, and call it
/// @param msgSize the size of the message
static void callbackTopic(size_t msgSize)
{
    int32_t errorCode = U_ERROR_COMMON_NOT_FOUND;
    for(int i=0; i<topicCallbackCount; i++) {
        if (strcmp(topicCallbackRegister[i]->topicName, topicString) == 0) {
            errorCode = runCommandCallback(topicCallbackRegister[i]->callbacks,
                                                topicCallbackRegister[i]->numCallbacks,
                                                downlinkMessage,
                                                msgSize);
        }
    }

    if (errorCode == U_ERROR_COMMON_NOT_FOUND)
        printWarn("Topic name not found in topic callback register: %s", topicString);
    else if (errorCode < 0)
        printWarn("Topic command callback failed: %d", errorCode);
}

/// @brief Go through the number of messages we have to read and read them
static void readMessages(void)
{
    int32_t count = messagesToRead;

    printDebug("MQTT Messages to read: %d", count);
    for(int i=0; i<count; i++) {
        size_t msgSize = readMessage();
        if (msgSize >= 0) {
            callbackTopic(msgSize);
            messagesToRead--;
        } else {
            // failure to read an MQTT message normally means
            // we don't have any more messages to read
            messagesToRead = 0;
            return;
        }
    }
}

/// @brief Task loop for the MQTT management
/// @param pParameters
static void taskLoop(void *pParameters)
{
    U_PORT_MUTEX_LOCK(TASK_MUTEX);
    while(isNotExiting())
    {
        if (!uMqttClientIsConnected(pContext)) {
            gAppStatus = MQTT_DISCONNECTED;
            if (gIsNetworkUp && gIsNetworkSignalValid) {
                writeLog("MQTT client disconnected, going to try to connect...");
                connectBroker();
            } else {
                writeDebug("Can't connect to MQTT broker, network is still not available...");
                uPortTaskBlock(2000);
            }
        } else {
            if (messagesToRead > 0)
                readMessages();

            dwellTask(taskConfig, continueToDwell);
        }
    }

    // Application exiting, so disconnect from MQTT Broker...
    Z_SECTION_LOCK
        disconnectBroker();
        uMqttClientClose(pContext);

        freeCallbacks();
        uPortFree(downlinkMessage);
        downlinkMessage = NULL;
    Z_SECTION_UNLOCK

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
    FINALISE_TASK;
}

static int32_t initQueue()
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(mqttMsg_t),
                    MQTT_QUEUE_STACK_SIZE,
                    MQTT_QUEUE_PRIORITY,
                    MQTT_QUEUE_SIZE);

    if (eventQueueHandle < 0) {
        writeFatal("Failed to create MQTT event queue %d.", eventQueueHandle);
    }

    TASK_QUEUE = eventQueueHandle;

    return eventQueueHandle;
}

static int32_t initMutex()
{
    INIT_MUTEX;
}

/// @brief Register a callback based on the topic of the message
/// @param topicName The topic of interest
/// @param callbackFunction The callback functaion to call when we received a message
/// @return 0 on success, negative on failure
static int32_t registerTopicCallBack(topicCallback_t *topicCallback)
{
    if (topicCallbackCount == MAX_TOPIC_CALLBACKS) {
        writeError("Maximum number of topic callbacks reached.");
        return U_ERROR_COMMON_NO_MEMORY;
    }

    if (pContext == NULL || !uMqttClientIsConnected(pContext)) {
        return U_ERROR_COMMON_NOT_INITIALISED;
    }

    int32_t errorCode = uMqttClientSubscribe(pContext, topicCallback->topicName, topicCallback->qos);
    if (errorCode < 0) {
        writeError("Failed to subscribe to topic: %s", topicCallback->topicName);
        return errorCode;
    }

    topicCallbackRegister[topicCallbackCount] = topicCallback;
    topicCallbackCount++;

    return U_ERROR_COMMON_SUCCESS;
}

static void subscribeToTopic(void *pParam)
{
    topicCallback_t *topicCallback = (topicCallback_t *)pParam;

    printDebug("Subscribing to topic '%s'...", topicCallback->topicName);

    // wait until the MQTT has been initialised...
    while(!TASK_INITIALISED && !gExitApp) {
        printDebug("Waiting for MQTT Task to be initialised...");
        uPortTaskBlock(500);
    }

    // wait until the MQTT Task is up and running...
    while(!isMutexLocked(TASK_MUTEX) && !gExitApp) {
        printDebug("Waiting for MQTT Task to start...");
        uPortTaskBlock(500);
    }

    printDebug("Finished waiting for MQTT task to start...");

    // the MQTT task is running, but we might not be connected yet so this can fail
    // U_ERROR_COMMON_NOT_INITIALISED is the error if the MQTT client isn't connected yet.
    int32_t errorCode = U_ERROR_COMMON_NOT_INITIALISED;
    while(errorCode == U_ERROR_COMMON_NOT_INITIALISED && !gExitApp) {
        errorCode = registerTopicCallBack(topicCallback);
        if(errorCode == U_ERROR_COMMON_NOT_INITIALISED) {
            printDebug("MQTT not connected yet, waiting another 5 seconds...");
            uPortTaskBlock(5000);
        }
    }

    if (errorCode != 0) {
        writeError("Subscribing a callback to topic %s failed with erorr code %d", topicCallback->topicName, errorCode);
        goto cleanUp;
    }

    Z_SECTION_LOCK
        writeLog("Subscribed to callback topic: %s", topicCallback->topicName);
        printLog("With these commands:");
        for(int i=0; i<topicCallback->numCallbacks; i++)
            printLog("%s", topicCallback->callbacks[i].command);
    Z_SECTION_UNLOCK

cleanUp:
    uPortTaskDelete(NULL);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Subscribes a callback function to a topic, waiting for the MQTT task to be online first
/// @param taskTopicName The topic name to subscribe to. Appends the serial number
/// @param qos The Quality of Service to use for the subscription
/// @param callbacks The callbacks this topic is going to be used for
int32_t subscribeToTopicAsync(const char *taskTopicName, uMqttQos_t qos, callbackCommand_t *callbacks, int32_t numCallbacks)
{
    int32_t errorCode = U_ERROR_COMMON_SUCCESS;
    uPortTaskHandle_t handle;
    topicCallback_t *topicCallbackInfo = NULL;
    char *topicName = NULL;

    topicCallbackInfo = (topicCallback_t *) pUPortMalloc(sizeof(topicCallback_t));
    if (topicCallbackInfo == NULL) {
        writeError("Failed to create topicCallback - not enough memory");
        errorCode = U_ERROR_COMMON_NO_MEMORY;
        goto cleanup;
    }

    snprintf(tempTopicName, TEMP_TOPIC_NAME_SIZE, "/%s/%s", gSerialNumber, taskTopicName);
    topicName = uStrDup(tempTopicName);
    if (topicName == NULL) {
        writeError("Failed to create task topic name - not enough memory");
        errorCode = U_ERROR_COMMON_NO_MEMORY;
        goto cleanup;
    }

    topicCallbackInfo->topicName = topicName;
    topicCallbackInfo->qos = qos;
    topicCallbackInfo->numCallbacks = numCallbacks;
    topicCallbackInfo->callbacks = callbacks;

    errorCode = uPortTaskCreate(subscribeToTopic, NULL, 2048, (void *)topicCallbackInfo, 5, &handle);
    if (errorCode != 0) {
        writeError("Can't start topic subscription on %s: %d", taskTopicName, errorCode);
        goto cleanup;
    }

cleanup:
    if (errorCode != 0) {
        uPortFree(topicCallbackInfo);
        topicCallbackInfo = NULL;

        uPortFree(topicName);
        topicName = NULL;
    }

    return errorCode;
}

/// @brief Puts a message on to the MQTT publish queue
/// @param pTopicName a pointer to the topic name which is copied
/// @param pMessage a pointer to the message text which is copied
/// @param QoS the Quality of Service value for this message
/// @param retain If the message should be retained
/// @return 0 if successfully queued on the event queue
int32_t sendMQTTMessage(const char *pTopicName, const char *pMessage, uMqttQos_t QoS, bool retain)
{
    // if the event queue handle is not valid, don't send the message
    if (TASK_QUEUE < 0) {
        writeWarn("MQTT Event Queue handle is not valid, not publishing MQTT message");
        return U_ERROR_COMMON_NOT_INITIALISED;
    }

    if (!IS_NETWORK_AVAILABLE) {
        writeWarn("Not publishing MQTT message, Network is not available at the moment");
        return U_ERROR_COMMON_TEMPORARY_FAILURE;
    }

    int32_t errorCode = U_ERROR_COMMON_SUCCESS;

    mqttMsg_t qMsg;
    qMsg.msgType = SEND_MQTT_MESSAGE;

    bool failed = false;
    failed = COPYTO(qMsg.msg.message, pTopicName);
    failed = COPYTO(qMsg.msg.message, pMessage);

    if (failed) {
        errorCode = U_ERROR_COMMON_NO_MEMORY;
        writeLog("Failed to allocate memory for MQTT message.");
        goto cleanup;
    }

    qMsg.msg.message.QoS = QoS;
    qMsg.msg.message.retain = retain;

    errorCode = uPortEventQueueSendIrq(TASK_QUEUE, &qMsg, sizeof(mqttMsg_t));
    if (errorCode != 0) {
        writeLog("Failed to send Event Queue Message: %d", errorCode);
        goto cleanup;
    }

cleanup:
    if (errorCode != 0) {
        uPortFree(qMsg.msg.message.pTopicName);
        qMsg.msg.message.pTopicName = NULL;

        uPortFree(qMsg.msg.message.pMessage);
        qMsg.msg.message.pMessage = NULL;
    }

    return errorCode;
}

/// @brief Initialises the MQTT task
/// @param config The task configuration structure
/// @return zero if successfull, a negative number otherwise
int32_t initMQTTTask(taskConfig_t *config)
{
    EXIT_IF_CONFIG_NULL;

    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Initializing the %s task...", TASK_NAME);
    CHECK_SUCCESS(initMutex);
    CHECK_SUCCESS(initQueue);

    return result;
}

/// @brief Starts the Signal Quality task loop
/// @return zero if successfull, a negative number otherwise
int32_t startMQTTTaskLoop(commandParamsList_t *params)
{
    EXIT_IF_CANT_RUN_TASK;

    int32_t errorCode = U_ERROR_COMMON_SUCCESS;

    pContext = pUMqttClientOpen(gDeviceHandle, NULL);
    if (pContext == NULL) {
        writeFatal("Failed to open the MQTT client");
        errorCode = U_ERROR_COMMON_NOT_RESPONDING;
        goto cleanUp;
    }

    errorCode = uPortTaskCreate(runTaskAndDelete,
                                TASK_NAME,
                                MQTT_TASK_STACK_SIZE,
                                taskLoop,
                                MQTT_TASK_PRIORITY,
                                &TASK_HANDLE);
    if (errorCode != 0) {
        writeError("Failed to start the %s Task (%d).", TASK_NAME, errorCode);
        goto cleanUp;
    }

    downlinkMessage = pUPortMalloc(MAX_MESSAGE_SIZE);
    if (downlinkMessage == NULL) {
        writeFatal("Failed to allocate MQTT downlink message buffer");
        errorCode = U_ERROR_COMMON_NO_MEMORY;
        goto cleanUp;
    }

cleanUp:
    if (errorCode != 0) {
        uPortFree(downlinkMessage);
        downlinkMessage = NULL;
    }

    return errorCode;
}

int32_t stopMQTTTaskLoop(commandParamsList_t *params)
{
    STOP_TASK;
}
