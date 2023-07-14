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

#define MQTT_QUEUE_STACK_SIZE QUEUE_STACK_SIZE_DEFAULT
#define MQTT_QUEUE_PRIORITY 5
#define MQTT_QUEUE_SIZE 10

#define STRCOPYTO(x, y)         (failed ? true : ((x = uStrDup(y))==NULL) ? true : false)
#define MEMCOPYTO(x, y, len)    (failed ? true : ((x = uMemDup(y, len))==NULL) ? true : false)

#define MAX_TOPIC_SIZE 100
#define MAX_MESSAGE_SIZE (12 * 1024 + 1)    // set this to 12KB as this
                                            // is the same buffer size
                                            // in the modules plus 1
                                            // for the null

#define MAX_TOPIC_CALLBACKS 50

#define TEMP_TOPIC_NAME_SIZE 150

#define MQTT_TYPE_NAME (mqttSN ? "MQTT-SN Gateway" : "MQTT Broker")

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
    uMqttSnTopicName_t *snShortName;
    uMqttQos_t qos;

    int32_t numCallbacks;
    callbackCommand_t *callbacks;
} topicCallback_t;

typedef struct MQTTSN_TOPIC_NAME_NODE {
    char *topicName;
    uMqttSnTopicName_t *snShortName;
    struct MQTTSN_TOPIC_NAME_NODE *next;
} mqttSNTopicNameNode_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static uMqttClientContext_t *pContext = NULL;
static uSecurityTlsSettings_t tlsSettings = U_SECURITY_TLS_SETTINGS_DEFAULT;
static uSecurityTlsCipherSuites_t cipherSuites;

static int32_t messagesToRead = 0;
static char topicString[MAX_TOPIC_SIZE];
static char *downlinkMessage;

static int32_t topicCallbackCount = 0;
static topicCallback_t *topicCallbackRegister[MAX_TOPIC_CALLBACKS];

static char tempTopicName[TEMP_TOPIC_NAME_SIZE];

static bool mqttSN = false;
static mqttSNTopicNameNode_t *mqttSNTopicNameList = NULL;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static bool isNotExiting(void)
{
    return !gExitApp && !exitTask;
}

/// @brief Send an MQTT Message - remember to FREE the msg memory!!!!
/// @param msg The message to send.
static void mqttSendMessage(sendMQTTMsg_t msg)
{
    if (!isNotExiting()) return;

    int32_t errorCode = U_ERROR_COMMON_NOT_INITIALISED;

    bool mqttConnected = uMqttClientIsConnected(pContext);
    if (pContext != NULL && mqttConnected && IS_NETWORK_AVAILABLE) {
        if (mqttSN) {
            errorCode = uMqttClientSnPublish(pContext, msg.topic.pShortName, msg.pMessage,
                                                    strlen(msg.pMessage),
                                                    msg.QoS,
                                                    msg.retain);
        } else {
            errorCode = uMqttClientPublish(pContext, msg.topic.pTopicName, msg.pMessage,
                                                    strlen(msg.pMessage),
                                                    msg.QoS,
                                                    msg.retain);
        }

        if (errorCode == 0) {
            writeDebug("Published MQTT message");
        } else {
            int32_t errValue = uMqttClientGetLastErrorCode(pContext);
            writeWarn("Failed to publish MQTT message, %s error: ", errValue);
        }

    } else {
        writeWarn("Network or MQTT connection not available, not publishing message");
    }

    gAppStatus = mqttConnected ? MQTT_CONNECTED : MQTT_DISCONNECTED;

    uPortFree(msg.pMessage);
    if (mqttSN)
        uPortFree(msg.topic.pShortName);
    else
        uPortFree(msg.topic.pTopicName);
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    if (!isNotExiting()) return;

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

    uMqttClientConnection_t connection = U_MQTT_CLIENT_CONNECTION_DEFAULT;
    connection.pBrokerNameStr = getConfig("MQTT_BROKER_NAME");
    connection.pUserNameStr = getConfig("MQTT_USERNAME");
    connection.pPasswordStr = getConfig("MQTT_PASSWORD");
    connection.pClientIdStr = getConfig("MQTT_CLIENTID");

    setBoolParamFromConfig("MQTT_TYPE", "MQTT-SN", &mqttSN);
    connection.mqttSn = mqttSN;

    setIntParamFromConfig("MQTT_TIMEOUT", &(connection.inactivityTimeoutSeconds));
    setBoolParamFromConfig("MQTT_KEEPALIVE", "TRUE", &(connection.keepAlive));

    writeLog("Connecting to %s on %s...", MQTT_TYPE_NAME, connection.pBrokerNameStr);

    int32_t errorCode = uMqttClientConnect(pContext, &connection);
    if (errorCode != 0) {
        writeError("Failed to connect to the %s: %d", MQTT_TYPE_NAME, errorCode);
        return errorCode;
    }

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

    writeLog("Connected to %s", MQTT_TYPE_NAME);
    gAppStatus = MQTT_CONNECTED;

    return 0;
}

static int32_t disconnectBroker(void)
{
    int32_t errorCode = uMqttClientDisconnect(pContext);
    if (errorCode != 0) {
        writeError("Failed to disconnect from %s: %d", MQTT_TYPE_NAME, errorCode);
    } else {
        writeLog("Disconnected from %s", MQTT_TYPE_NAME);
    }

    return errorCode;
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

static bool getTopicNameFromSnTopicId(uint16_t id, char *topicName)
{
    for(int i=0; i<topicCallbackCount; i++) {
        if (topicCallbackRegister[i]->snShortName->name.id == id) {
            memcpy(topicName, topicCallbackRegister[i]->topicName, strlen(topicCallbackRegister[i]->topicName));
            return true;
        }
    }

    return false;
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
    if (mqttSN) {
        uMqttSnTopicName_t snTopicName;
        errorCode = uMqttClientSnMessageRead(pContext, &snTopicName, downlinkMessage, &msgSize, &QoS);
        if (!getTopicNameFromSnTopicId(snTopicName.name.id, topicString)) {
            printWarn("Failed to find MQTT-SN TopicId: %d", snTopicName.name.id);
            errorCode = U_ERROR_COMMON_NOT_FOUND;
        }
    } else {
        errorCode = uMqttClientMessageRead(pContext, topicString, MAX_TOPIC_SIZE, downlinkMessage, &msgSize, &QoS);
    }

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
        printWarn("callbackTopic(): Topic name %s not found", topicString);
    else if (errorCode < 0)
        printWarn("callbackTopic(): Topic command callback failed: %d", errorCode);
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
                writeLog("MQTT client disconnected, trying to connect...");
                if (connectBroker() != U_ERROR_COMMON_SUCCESS)
                    uPortTaskBlock(5000);
            } else {
                writeDebug("Can't connect to %s, network is still not available...", MQTT_TYPE_NAME);
                uPortTaskBlock(2000);
            }
        } else {
            if (messagesToRead > 0)
                readMessages();

            dwellTask(taskConfig, continueToDwell);
        }
    }

    // Application exiting, so disconnect from MQTT broker/SN gateway...
    disconnectBroker();
    uMqttClientClose(pContext);

    freeCallbacks();
    uPortFree(downlinkMessage);
    downlinkMessage = NULL;

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
    FINALIZE_TASK;
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
        writeError("registerTopicCallBack(): max callback count");
        return U_ERROR_COMMON_NO_MEMORY;
    }

    if (pContext == NULL || !uMqttClientIsConnected(pContext)) {
        return U_ERROR_COMMON_NOT_INITIALISED;
    }

    int32_t errorCode;

    if (mqttSN) {
        topicCallback->snShortName = (uMqttSnTopicName_t *)malloc(sizeof(uMqttSnTopicName_t));
        if (topicCallback->snShortName == NULL) {
            writeError("registerTopicCallBack(): snShortName memory allocation");
            return U_ERROR_COMMON_NO_MEMORY;
        }

        errorCode = uMqttClientSnSubscribeNormalTopic(pContext, topicCallback->topicName,
                                                                topicCallback->qos,
                                                                topicCallback->snShortName);
    } else {
        errorCode = uMqttClientSubscribe(pContext,  topicCallback->topicName,
                                                    topicCallback->qos);
    }

    if (errorCode < 0) {
        writeError("registerTopicCallBack(): Subscribe to topic %s", topicCallback->topicName);
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
            printDebug("Still waiting to subscribe to %s topic");
            uPortTaskBlock(5000);
        }
    }

    if (errorCode != 0 ) {
        if (!gExitApp)
            writeError("Subscribing a callback to topic %s failed with error code %d", topicCallback->topicName, errorCode);
    
        goto cleanUp;
    }

    writeLog("Subscribed to callback topic: %s", topicCallback->topicName);
    if (topicCallback->numCallbacks > 0) {
        printLog("With these commands:");
        for(int i=0; i<topicCallback->numCallbacks; i++)
            printLog("    %d: %s", i+1, topicCallback->callbacks[i].command);
        printLog("");
    } else {
        printWarn("Warning - there are no commands to listen to on this subscription!");
    }

cleanUp:
    uPortTaskDelete(NULL);
}

static int32_t registerSNShortName(const char *topicName, uMqttSnTopicName_t **snShortName)
{
    int32_t errorCode = U_ERROR_COMMON_NO_MEMORY;

    // allocate the memory for the sn Topic Name structure
    *snShortName = (uMqttSnTopicName_t *)malloc(sizeof(uMqttSnTopicName_t));
    if (*snShortName == NULL) {
        writeError("registerSNShortName(): snShortName memory allocation");
        goto cleanUp;
    }

    // register the topic name with the MQTT-SN gateway
    errorCode = uMqttClientSnRegisterNormalTopic(pContext, topicName, *snShortName);
    if (errorCode != 0) {
        writeError("registerSNShortName(): Register Normal Topic '%s': %d", topicName, errorCode);
        goto cleanUp;
    }

    errorCode = U_ERROR_COMMON_SUCCESS;

cleanUp:
    if (errorCode != 0) {
        uPortFree(*snShortName);
        *snShortName = NULL;
    }

    return errorCode;
}

static int32_t getMqttSNTopicName(const char *topicName, uMqttSnTopicName_t **snShortName)
{
    mqttSNTopicNameNode_t *currentNode = mqttSNTopicNameList;

    // check if the short name has already been registered?
    while(currentNode != NULL) {
        if(strcmp(currentNode->topicName, topicName) == 0) {
            *snShortName = currentNode->snShortName;
            return U_ERROR_COMMON_SUCCESS;
        }

        currentNode = currentNode->next;
    }

    // Create a new topicName/shortName node and register the topic with the MQTT-SN gateway
    int32_t errorCode = U_ERROR_COMMON_NO_MEMORY;
    mqttSNTopicNameNode_t *newNode = (mqttSNTopicNameNode_t *)malloc(sizeof(mqttSNTopicNameNode_t));
    if (newNode == NULL) {
        writeError("getMqttSNTopicName(): newNode memory allocation");
        goto cleanUp;
    }

    newNode->topicName = uStrDup(topicName);
    if (newNode->topicName == NULL) {
        writeError("getMqttSNTopicName(): topicName memory allocation");
        goto cleanUp;
    }

    errorCode = registerSNShortName(topicName, snShortName);
    if (errorCode < 0)
        goto cleanUp;

    // add it to the linked list
    newNode->snShortName = *snShortName;
    newNode->next = mqttSNTopicNameList;
    mqttSNTopicNameList = newNode;

    errorCode = U_ERROR_COMMON_SUCCESS;

cleanUp:
    if (errorCode != 0) {
        uPortFree(newNode->topicName);
        uPortFree(newNode);
    }

    return errorCode;
}

static void setSecuritySettings(void)
{
    int32_t cert_value_level;
    setIntParamFromConfig("SECURITY_CERT_VALID_LEVEL", &cert_value_level);
    tlsSettings.certificateCheck = cert_value_level;

    int32_t tls_version;
    setIntParamFromConfig("SECURITY_TLS_VERSION", &tls_version);
    tlsSettings.tlsVersionMin = tls_version;

    int32_t cipher;
    setIntParamFromConfig("SECURITY_CIPHER_SUITE", &cipher);
    if (cipher == 0) {
        cipherSuites.num = 0;
    } else {
        cipherSuites.num = 1;
        cipherSuites.suite[0] = cipher;
    }
    tlsSettings.cipherSuites = cipherSuites;
    
    const char *client_name = getConfig("SECURITY_CLIENT_NAME");
    tlsSettings.pClientCertificateName = client_name;

    const char *client_key = getConfig("SECURITY_CLIENT_KEY");
    tlsSettings.pClientPrivateKeyName = client_key;
    
    const char *server_name_ind = getConfig("SECURITY_SERVER_NAME_IND");
    tlsSettings.pSni = server_name_ind;
}

static int32_t initMQTTClient(void)
{
    int32_t errorCode = U_ERROR_COMMON_SUCCESS;

    downlinkMessage = pUPortMalloc(MAX_MESSAGE_SIZE);
    if (downlinkMessage == NULL) {
        writeFatal("Failed to allocate MQTT downlink message buffer");
        errorCode = U_ERROR_COMMON_NO_MEMORY;
        goto cleanUp;
    }

    bool security = false;
    setBoolParamFromConfig("MQTT_SECURITY", "TRUE", &security);
    if (security) {
        setSecuritySettings();
        pContext = pUMqttClientOpen(gDeviceHandle, &tlsSettings);
    }
    else
        pContext = pUMqttClientOpen(gDeviceHandle, NULL);
    
    if (pContext == NULL) {
        writeFatal("Failed to open the MQTT client");
        errorCode = U_ERROR_COMMON_NOT_RESPONDING;
        goto cleanUp;
    }

cleanUp:
    if (errorCode != 0) {
        uPortFree(downlinkMessage);
        downlinkMessage = NULL;
    }

    return errorCode;
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
        goto cleanUp;
    }

    snprintf(tempTopicName, TEMP_TOPIC_NAME_SIZE, "/%s/%s", gSerialNumber, taskTopicName);
    topicName = uStrDup(tempTopicName);
    if (topicName == NULL) {
        writeError("Failed to create task topic name - not enough memory");
        errorCode = U_ERROR_COMMON_NO_MEMORY;
        goto cleanUp;
    }

    topicCallbackInfo->topicName = topicName;
    topicCallbackInfo->qos = qos;
    topicCallbackInfo->numCallbacks = numCallbacks;
    topicCallbackInfo->callbacks = callbacks;

    errorCode = uPortTaskCreate(subscribeToTopic, NULL, 2048, (void *)topicCallbackInfo, 5, &handle);
    if (errorCode != 0) {
        writeError("Can't start topic subscription on %s: %d", taskTopicName, errorCode);
        goto cleanUp;
    }

cleanUp:
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
        writeWarn("Not publishing MQTT message, MQTT Event Queue handle is not valid");
        return U_ERROR_COMMON_NOT_INITIALISED;
    }

    if (!IS_NETWORK_AVAILABLE) {
        writeWarn("Not publishing MQTT message, Network is not available at the moment");
        return U_ERROR_COMMON_TEMPORARY_FAILURE;
    }

    if (pContext == NULL || !uMqttClientIsConnected(pContext)) {
        writeWarn("Not publishing MQTT message, not connected to %s", MQTT_TYPE_NAME);
        return U_ERROR_COMMON_NOT_INITIALISED;
    }

    if (!isNotExiting()) return U_ERROR_COMMON_BUSY;

    int32_t errorCode = U_ERROR_COMMON_SUCCESS;

    mqttMsg_t qMsg;
    qMsg.msgType = SEND_MQTT_MESSAGE;

    bool failed = false;
    failed = STRCOPYTO(qMsg.msg.message.pMessage, pMessage);
    if (!failed && mqttSN) {
        uMqttSnTopicName_t *snShortName;
        if (getMqttSNTopicName(pTopicName, &snShortName) < 0) {
            writeError("Not publishing MQTT-SN message, failed to get/register MQTT-SN Topic Name.");
            goto cleanUp;
        }

        failed = MEMCOPYTO(qMsg.msg.message.topic.pShortName, snShortName, sizeof(uMqttSnTopicName_t));
    } else {
        failed = STRCOPYTO(qMsg.msg.message.topic.pTopicName, pTopicName);
    }

    if (failed) {
        errorCode = U_ERROR_COMMON_NO_MEMORY;
        writeLog("Not publishing MQTT message, failed to allocate memory for message.");
        goto cleanUp;
    }

    qMsg.msg.message.QoS = QoS;
    qMsg.msg.message.retain = retain;

    errorCode = uPortEventQueueSendIrq(TASK_QUEUE, &qMsg, sizeof(mqttMsg_t));
    if (errorCode != 0) {
        writeLog("Not publishing MQTT message, Event Queue Full");
        goto cleanUp;
    }

cleanUp:
    if (errorCode != 0) {
        if (mqttSN) {
            uPortFree(qMsg.msg.message.topic.pShortName);
            qMsg.msg.message.topic.pShortName = NULL;
        } else {
            uPortFree(qMsg.msg.message.topic.pTopicName);
            qMsg.msg.message.topic.pTopicName = NULL;
        }

        uPortFree(qMsg.msg.message.pMessage);
        qMsg.msg.message.pMessage = NULL;
    }

    return errorCode;
}

/// @brief Initialises the MQTT task
/// @param config The task configuration structure
/// @return zero if successful, a negative number otherwise
int32_t initMQTTTask(taskConfig_t *config)
{
    EXIT_IF_CONFIG_NULL;

    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Initializing the %s task...", TASK_NAME);
    EXIT_ON_FAILURE(initMutex);
    EXIT_ON_FAILURE(initQueue);
    EXIT_ON_FAILURE(initMQTTClient);

    return result;
}

/// @brief Starts the Signal Quality task loop
/// @return zero if successful, a negative number otherwise
int32_t startMQTTTaskLoop(commandParamsList_t *params)
{
    EXIT_IF_CANT_RUN_TASK;

    int32_t errorCode = U_ERROR_COMMON_SUCCESS;

    errorCode = uPortTaskCreate(runTaskAndDelete,
                                TASK_NAME,
                                MQTT_TASK_STACK_SIZE,
                                taskLoop,
                                MQTT_TASK_PRIORITY,
                                &TASK_HANDLE);
    if (errorCode != 0) {
        writeError("Failed to start the %s Task (%d).", TASK_NAME, errorCode);
    }

    return errorCode;
}

int32_t stopMQTTTaskLoop(commandParamsList_t *params)
{
    STOP_TASK;
}
