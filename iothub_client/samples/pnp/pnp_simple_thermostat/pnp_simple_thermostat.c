// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample implements an IoT Plug and Play based thermostat.  This demonstrates a *relatively* simple PnP device
// that does only acts as a thermostat and does not have additional components.  

// The DigitalTwin Definition Language document describing the component implemented in this sample
// is available at https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json.

// Standard C header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// IoTHub Device Client and IoT core utility related header files
#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_message.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "iothub_client_properties.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"


#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "azure_c_shared_utility/shared_util_options.h"
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#include "pnp_sample_config.h"

#ifdef USE_PROV_MODULE_FULL
#include "pnp_dps_ll.h"
#endif // USE_PROV_MODULE_FULL

// JSON parser library
#include "parson.h"

// Values of connection / security settings read from environment variables and/or DPS runtime
PNP_DEVICE_CONFIGURATION g_pnpDeviceConfiguration;

// Amount of time to sleep between polling hub, in milliseconds.  Set to wake up every 100 milliseconds.
static unsigned int g_sleepBetweenPollsMs = 100;

// Every time the main loop wakes up, on the g_sendTelemetryPollInterval(th) pass will send a telemetry message.
// So we will send telemetry every (g_sendTelemetryPollInterval * g_sleepBetweenPollsMs) milliseconds; 60 seconds as currently configured.
static const int g_sendTelemetryPollInterval = 600;

// Whether verbose tracing at the IoTHub client is enabled or not.
static bool g_hubClientTraceEnabled = true;

// This device's PnP ModelId.
static const char g_ThermostatModelId[] = "dtmi:com:example:Thermostat;1";

// Names of properties for desired/reporting
static const char g_targetTemperaturePropertyName[] = "targetTemperature";
static const char g_maxTempSinceLastRebootPropertyName[] = "maxTempSinceLastReboot";

// Name of command this component supports to get report information
static const char g_getMaxMinReport[] = "getMaxMinReport";
// Return codes for commands and desired property responses
static int g_statusSuccess = 200;
static int g_statusBadFormat = 400;
static int g_statusNotFoundStatus = 404;
static int g_statusInternalError = 500;

// An empty JSON body for PnP command responses
static const char g_JSONEmpty[] = "{}";
static const size_t g_JSONEmptySize = sizeof(g_JSONEmpty) - 1;

// The default temperature to use before any is set.
#define DEFAULT_TEMPERATURE_VALUE 22
// Current temperature of the thermostat.
static double g_currentTemperature = DEFAULT_TEMPERATURE_VALUE;
// Minimum temperature thermostat has been at during current execution run.
static double g_minTemperature = DEFAULT_TEMPERATURE_VALUE;
// Maximum temperature thermostat has been at during current execution run.
static double g_maxTemperature = DEFAULT_TEMPERATURE_VALUE;
// Number of times temperature has been updated, counting the initial setting as 1.  Used to determine average temperature.
static int g_numTemperatureUpdates = 1;
// Total of all temperature updates during current execution run.  Used to determine average temperature.
static double g_allTemperatures = DEFAULT_TEMPERATURE_VALUE;

// snprintf format for building getMaxMinReport
static const char g_maxMinCommandResponseFormat[] = "{\"maxTemp\":%.2f,\"minTemp\":%.2f,\"avgTemp\":%.2f,\"startTime\":\"%s\",\"endTime\":\"%s\"}";

// Format string for sending temperature telemetry
static const char g_temperatureTelemetryBodyFormat[] = "{\"temperature\":%.02f}";

// Format string for sending maxTempSinceLastReboot property
static const char g_maxTempSinceLastRebootPropertyFormat[] = "%.2f";
// Format of the body when responding to a targetTemperature 
static const char g_targetTemperaturePropertyResponseFormat[] = "%.2f";

// Response description is an optional, human readable message including more information
// about the setting of the temperature.  On success cases, this sample does not 
// send a description to save bandwidth but on error cases we'll provide some hints.
static const char g_temperaturePropertyResponseDescriptionNotInt[] = "desired temperature is not a number";

// Size of buffer to store ISO 8601 time.
#define TIME_BUFFER_SIZE 128
// Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
static const char g_ISO8601Format[] = "%Y-%m-%dT%H:%M:%SZ";
// Start time of the program, stored in ISO 8601 format string for UTC.
char g_ProgramStartTime[TIME_BUFFER_SIZE];

// Size of buffer to store current temperature telemetry.
#define CURRENT_TEMPERATURE_BUFFER_SIZE  32

// Size of buffer to store the maximum temp since reboot property.
#define MAX_TEMPERATURE_SINCE_REBOOT_BUFFER_SIZE 32


//
// CopyPayloadToString creates a null-terminated string out of a buffer which
// is not guaranteed to be null-terminated by the IoT Hub device SDK.
//
static char* CopyPayloadToString(const unsigned char* payload, size_t size)
{
    char* jsonStr;

    if ((jsonStr = (char*)malloc(size+1)) == NULL)
    {
        LogError("Unable to allocate %lu size buffer", (unsigned long)(size+1));
    }
    else
    {
        memcpy(jsonStr, payload, size);
        jsonStr[size] = '\0';
    }

    return jsonStr;
}

//
// BuildUtcTimeFromCurrentTime writes the current time, in ISO 8601 format, into the specified buffer
//
static bool BuildUtcTimeFromCurrentTime(char* utcTimeBuffer, size_t utcTimeBufferSize)
{
    bool result;
    time_t currentTime;
    struct tm * currentTimeTm;

    time(&currentTime);
    currentTimeTm = gmtime(&currentTime);

    if (strftime(utcTimeBuffer, utcTimeBufferSize, g_ISO8601Format, currentTimeTm) == 0)
    {
        LogError("snprintf on UTC time failed");
        result = false;
    }
    else
    {
        result = true;
    }

    return result;
}

//
// BuildMaxMinCommandResponse builds the response to the command for getMaxMinReport
//
static bool BuildMaxMinCommandResponse(unsigned char** response, size_t* responseSize)
{
    int responseBuilderSize = 0;
    unsigned char* responseBuilder = NULL;
    bool result;
    char currentTime[TIME_BUFFER_SIZE];

    if (BuildUtcTimeFromCurrentTime(currentTime, sizeof(currentTime)) == false)
    {
        LogError("Unable to output the current time");
        result = false;
    }
    else if ((responseBuilderSize = snprintf(NULL, 0, g_maxMinCommandResponseFormat, g_maxTemperature, g_minTemperature, g_allTemperatures / g_numTemperatureUpdates, g_ProgramStartTime, currentTime)) < 0)
    {
        LogError("snprintf to determine string length for command response failed");
        result = false;
    }
    // We MUST allocate the response buffer.  It is returned to the IoTHub SDK in the command callback and the SDK in turn sends this to the server.
    else if ((responseBuilder = calloc(1, responseBuilderSize + 1)) == NULL)
    {
        LogError("Unable to allocate %lu bytes", (unsigned long)(responseBuilderSize + 1));
        result = false;
    }
    else if ((responseBuilderSize = snprintf((char*)responseBuilder, responseBuilderSize + 1, g_maxMinCommandResponseFormat, g_maxTemperature, g_minTemperature, g_allTemperatures / g_numTemperatureUpdates, g_ProgramStartTime, currentTime)) < 0)
    {
        LogError("snprintf to output buffer for command response");
        result = false;
    }
    else
    {
        result = true;
    }

    if (result == true)
    {
        *response = responseBuilder;
        *responseSize = (size_t)responseBuilderSize;
        LogInfo("Response=<%s>", (const char*)responseBuilder);
    }
    else
    {
        free(responseBuilder);
    }

    return result;
}       

//
// SetEmptyCommandResponse sets the response to be an empty JSON.  IoT Hub needs
// legal JSON, regardless of error status, so if command implementation did not set this do so here.
//
static void SetEmptyCommandResponse(unsigned char** response, size_t* responseSize, int* result)
{
    if ((*response = calloc(1, g_JSONEmptySize)) == NULL)
    {
        LogError("Unable to allocate empty JSON response");
        *result = g_statusInternalError;
    }
    else
    {
        memcpy(*response, g_JSONEmpty, g_JSONEmptySize);
        *responseSize = g_JSONEmptySize;
        // We only overwrite the caller's result on error; otherwise leave as it was
    }
}

//
// Thermostat_CommandCallback is invoked by IoT SDK when a command arrives.
//
static int Thermostat_CommandCallback(const char* componentName, const char* commandName, const unsigned char* payload, size_t size, const char* payloadContentType, unsigned char** response, size_t* responseSize, void* userContextCallback)
{
    (void)userContextCallback;
    (void)payloadContentType; 

    char* jsonStr = NULL;
    JSON_Value* rootValue = NULL;
    const char* sinceStr;
    int result;

    LogInfo("Device command %s arrived", commandName);

    *response = NULL;
    *responseSize = 0;

    if (componentName != NULL)
    {
        LogError("This model only supports root components, but component %s was specified in command", componentName);
        result = g_statusNotFoundStatus;
    }
    else if (strcmp(commandName, g_getMaxMinReport) != 0)
    {
        LogError("Command name %s is not supported on this component", commandName);
        result = g_statusNotFoundStatus;
    }
    else if ((jsonStr = CopyPayloadToString(payload, size)) == NULL)
    {
        LogError("Unable to allocate twin buffer");
        result = g_statusInternalError;
    }
    else if ((rootValue = json_parse_string(jsonStr)) == NULL)
    {
        LogError("Unable to parse twin JSON");
        result = g_statusBadFormat;
    }
    // See caveats section in ../readme.md; we don't actually respect this sinceStr to keep the sample simple,
    // but want to demonstrate how to parse out in any case.
    else if ((sinceStr = json_value_get_string(rootValue)) == NULL)
    {
        LogError("Cannot retrieve since value");
        result = g_statusBadFormat;
    }
    else if (BuildMaxMinCommandResponse(response, responseSize) == false)
    {
        LogError("Unable to build response");
        result = g_statusInternalError;        
    }
    else
    {
        LogInfo("Returning success from command request");
        result = g_statusSuccess;
    }

    if (*response == NULL)
    {
        SetEmptyCommandResponse(response, responseSize, &result);
    }

    json_value_free(rootValue);
    free(jsonStr);

    return result;
}

//
// UpdateTemperatureAndStatistics updates the temperature and min/max/average values
//
static void UpdateTemperatureAndStatistics(double desiredTemp, bool* maxTempUpdated)
{
    if (desiredTemp > g_maxTemperature)
    {
        g_maxTemperature = desiredTemp;
        *maxTempUpdated = true;
    }
    else if (desiredTemp < g_minTemperature)
    {
        g_minTemperature = desiredTemp;
    }

    g_numTemperatureUpdates++;
    g_allTemperatures += desiredTemp;

    g_currentTemperature = desiredTemp;
}

//
// SendTargetTemperatureResponse sends a PnP property indicating the device has received the desired targeted temperature
//
static void SendTargetTemperatureResponse(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient, const char* desiredTempString, int responseStatus, int version, const char* description)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;

    // IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE specifies the response to a desired temperature.
    IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE desiredTemperatureResponse;
    memset(&desiredTemperatureResponse, 0, sizeof(desiredTemperatureResponse));
    // Specify the structure version (not to be confused with the $version on IoT Hub) to protect back-compat in case the structure adds fields.
    desiredTemperatureResponse.structVersion= IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_VERSION_1;
    // This represents the version of the request from IoT Hub.  It needs to be returned so service applications can determine
    // what current version of the writable property the device is currently using, as the server may update the property even when the device
    // is offline.
    desiredTemperatureResponse.ackVersion = version;
    // Result of request, which maps to HTTP status code.  For sample we'll always indicate success.
    desiredTemperatureResponse.result = responseStatus;
    desiredTemperatureResponse.name = g_targetTemperaturePropertyName;
    desiredTemperatureResponse.value = desiredTempString;
    desiredTemperatureResponse.description = description;

    unsigned char* propertySerialized;
    size_t propertySerializedLength;

    // The first step of reporting properties is to serialize it into an IoT Hub friendly format.  You can do this by either
    // implementing the PnP convention for building up the correct JSON or more simply to use IoTHubClient_Serialize_WritablePropertyResponse.
    if ((iothubClientResult = IoTHubClient_Serialize_WritablePropertyResponse(&desiredTemperatureResponse, 1, NULL, &propertySerialized, &propertySerializedLength)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to serialize updated property, error=%d", iothubClientResult);
    }
    // The output of IoTHubClient_Serialize_WritablePropertyResponse is sent to IoTHubDeviceClient_LL_SendPropertiesAsync to perform network I/O.
    else if ((iothubClientResult = IoTHubDeviceClient_LL_SendPropertiesAsync(deviceClient, propertySerialized, propertySerializedLength, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send updated property, error=%d", iothubClientResult);
    }
    else
    {
        LogInfo("Sending acknowledgement of property to IoTHub");
    }
    IoTHubClient_Serialize_Properties_Destroy(propertySerialized);
}

//
// SendMaxTemperatureSinceReboot reports a PnP property indicating the maximum temperature since the last reboot (simulated here by lifetime of executable)
//
static void SendMaxTemperatureSinceReboot(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    char maximumTemperatureAsString[MAX_TEMPERATURE_SINCE_REBOOT_BUFFER_SIZE];

    if (snprintf(maximumTemperatureAsString, sizeof(maximumTemperatureAsString), g_maxTempSinceLastRebootPropertyFormat, g_maxTemperature) < 0)
    {
        LogError("Unable to create max temp since last reboot string for reporting result");
    }
    else
    {
        IOTHUB_CLIENT_REPORTED_PROPERTY maxTempProperty = { IOTHUB_CLIENT_REPORTED_PROPERTY_VERSION_1, g_maxTempSinceLastRebootPropertyName, maximumTemperatureAsString };

        unsigned char* propertySerialized = NULL;
        size_t propertySerializedLength;

        // The first step of reporting properties is to serialize it into an IoT Hub friendly format.  You can do this by either
        // implementing the PnP convention for building up the correct JSON or more simply to use IoTHubClient_Serialize_ReportedProperties.
        if ((iothubClientResult = IoTHubClient_Serialize_ReportedProperties(&maxTempProperty, 1, NULL, &propertySerialized, &propertySerializedLength)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to serialize reported state, error=%d", iothubClientResult);
        }
        // The output of IoTHubClient_Serialize_ReportedProperties is sent to IoTHubDeviceClient_LL_SendPropertiesAsync to perform network I/O.
        else if ((iothubClientResult = IoTHubDeviceClient_LL_SendPropertiesAsync(deviceClient, propertySerialized, propertySerializedLength,  NULL, NULL)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to send reported state, error=%d", iothubClientResult);
        }
        else
        {
            LogInfo("Sending maximumTemperatureSinceLastReboot property to IoTHub for component"); // TODO: Not logging error code but should be.
        }
        IoTHubClient_Serialize_Properties_Destroy(propertySerialized);
    }
}

//
// Thermostat_ProcessTargetTemperature processes a writable update for desired temperature property.
//
static void Thermostat_ProcessTargetTemperature(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient, IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property, int propertiesVersion)
{
    char* next;
    double targetTemperature = strtol(property->value.str, &next, 10);
    if ((property->value.str == next) || (targetTemperature == LONG_MAX) || (targetTemperature == LONG_MIN))
    {
        LogError("Property %s is not a valid integer", property->value.str);
        SendTargetTemperatureResponse(deviceClient, property->value.str, g_statusBadFormat, propertiesVersion, g_temperaturePropertyResponseDescriptionNotInt);
    }
    else
    {
        LogInfo("Received targetTemperature = %f", targetTemperature);
        
        bool maxTempUpdated = false;
        UpdateTemperatureAndStatistics(targetTemperature, &maxTempUpdated);
        
        // The device needs to let the service know that it has received the targetTemperature desired property.
        SendTargetTemperatureResponse(deviceClient, property->value.str, g_statusSuccess, propertiesVersion, NULL);
        
        if (maxTempUpdated)
        {
            // If the Maximum temperature has been updated, we also report this as a property.
            SendMaxTemperatureSinceReboot(deviceClient);
        }
    }
}

//
// Thermostat_PropertiesCallback is invoked when properties arrive from the server.
//
static void Thermostat_PropertiesCallback(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,  const unsigned char* payload, size_t payloadLength, void* userContextCallback)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = (IOTHUB_DEVICE_CLIENT_LL_HANDLE)userContextCallback;
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE propertyIterator;
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY property;
    int propertiesVersion;
    IOTHUB_CLIENT_RESULT clientResult;

    // The properties arrive as a raw JSON buffer.  IoTHubClient_Deserialize_Properties_CreateIterator parses this into a more 
    // convenient form where each property can be processed at a time and in an easier to use C structure.
    if ((clientResult = IoTHubClient_Deserialize_Properties_CreateIterator(payloadType, payload, payloadLength, NULL, 0, &propertyIterator, &propertiesVersion)) != IOTHUB_CLIENT_OK)
    {
        LogError("IoTHubClient_Deserialize_Properties failed, error=%d", clientResult);
    }
    else
    {
        bool propertySpecified;
        property.structVersion = IOTHUB_CLIENT_DESERIALIZED_PROPERTY_VERSION_1;

        while ((clientResult = IoTHubClient_Deserialize_Properties_GetNextProperty(propertyIterator, &property, &propertySpecified)) == IOTHUB_CLIENT_OK)
        {
            if (propertySpecified == false)
            {
                break;
            }

            if (property.propertyType == IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE)
            {
                // We are iterating over a property that the device has previously sent to IoT Hub.
                //
                // There are scenarios where a device may use this, such as knowing whether the
                // given property has changed on the device and needs to be re-reported to IoT Hub.
                //
                // This sample doesn't do anything with this, so we'll continue on when we hit reported properties.
                continue;
            }

            // Process IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE propertyType, which means IoT Hub is configuring a property
            // on this device.
            //
            // If we receive a property the model does not support, log the condition locally but do not report this
            // back to IoT Hub.
            if (property.componentName != NULL) 
            {   
                LogError("Property=%s arrived for a non-root component.  This model does not support such properties", property.name);
            }
            else if (strcmp(property.componentName, g_targetTemperaturePropertyName) == 0)
            {
                Thermostat_ProcessTargetTemperature(deviceClient, &property, propertiesVersion);
            }
            else
            {
                LogError("Property=%s is not part of the thermostat model and will be ignored", property.name);
            }
            
            IoTHubClient_Deserialize_Properties_DestroyProperty(&property);
        }
    }

    IoTHubClient_Deserialize_Properties_DestroyIterator(propertyIterator);
}

//
// Thermostat_SendCurrentTemperature sends a PnP telemetry indicating the current temperature
//
void Thermostat_SendCurrentTemperature(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;

    char temperatureStringBuffer[CURRENT_TEMPERATURE_BUFFER_SIZE];

    if (snprintf(temperatureStringBuffer, sizeof(temperatureStringBuffer), g_temperatureTelemetryBodyFormat, g_currentTemperature) < 0)
    {
        LogError("snprintf of current temperature telemetry failed");
    }
    else if ((messageHandle = IoTHubMessage_CreateFromString(temperatureStringBuffer)) == NULL)
    {
        LogError("IoTHubMessage_CreateFromString failed");
    }
    else if ((iothubResult = IoTHubDeviceClient_LL_SendTelemetryAsync(deviceClient, messageHandle, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send telemetry message, error=%d", iothubResult);
    }

    IoTHubMessage_Destroy(messageHandle);
}

//
// CreateDeviceClientLLHandle performs actual handle creation (but nothing more), depending
// on whether connection strings or DPS is used.
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE CreateDeviceClientLLHandle(void)
{
#ifdef USE_PROV_MODULE_FULL
    if (g_pnpDeviceConfiguration.securityType == PNP_CONNECTION_SECURITY_TYPE_DPS)
    {
        // Pass the modelId to DPS here AND later on to IoT Hub (see SetOption on OPTION_MODEL_ID) when
        // that connection is created.  We need to do both because DPS does not auto-populate the modelId
        // it receives on DPS connection to the IoT Hub.
        g_pnpDeviceConfiguration.modelId = g_ThermostatModelId;
        g_pnpDeviceConfiguration.enableTracing = g_hubClientTraceEnabled;
        return PnP_CreateDeviceClientLLHandle_ViaDps(&g_pnpDeviceConfiguration);
    }
#endif
    return IoTHubDeviceClient_LL_CreateFromConnectionString(g_pnpDeviceConfiguration.u.connectionString, MQTT_Protocol);
}

//
// CreateAndConfigureDeviceClientHandleForPnP creates a IOTHUB_DEVICE_CLIENT_LL_HANDLE for this application, setting its
// ModelId along with various callbacks.
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE CreateAndConfigureDeviceClientHandleForPnP(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;
    bool urlAutoEncodeDecode = true;
    int iothubInitResult;
    bool result;

    // Before invoking any IoTHub Device SDK functionality, IoTHub_Init must be invoked.
    if ((iothubInitResult = IoTHub_Init()) != 0)
    {
        LogError("Failure to initialize client.  Error=%d", iothubInitResult);
        result = false;
    }
    // Create the deviceClient itself.
    else if ((deviceClient = CreateDeviceClientLLHandle()) == NULL)
    {
        LogError("Failure creating IotHub client.  Hint: Check your connection string or DPS configuration");
        result = false;
    }
    // Sets verbosity level
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_LOG_TRACE, &g_hubClientTraceEnabled)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set logging option, error=%d", iothubResult);
        result = false;
    }
    // Sets the name of ModelId for this PnP device.
    // This *MUST* be set before the client is connected to IoTHub.  We do not automatically connect when the 
    // handle is created, but will implicitly connect to subscribe for command and property callbacks below.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_MODEL_ID, g_ThermostatModelId)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the ModelID, error=%d", iothubResult);
        result = false;
    }
    // Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically for telemetry message properties.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set auto Url encode option, error=%d", iothubResult);
        result = false;
    }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on systems without built in certificate stores.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_TRUSTED_CERT, certificates)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the trusted cert, error=%d", iothubResult);
        result = false;
    }
#endif // SET_TRUSTED_CERT_IN_SAMPLES
    // Sets the callback function that processes incoming commands.  Note that this will implicitly initiate a connection to IoT Hub.
    else if ((iothubResult = IoTHubDeviceClient_LL_SubscribeToCommands(deviceClient, Thermostat_CommandCallback, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to subscribe for commands, error=%d", iothubResult);
        result = false;
    }
    // Sets the callback function that processes device twin changes from the IoTHub.  This will also automatically retrieve the full twin for the application. 
    else if ((iothubResult = IoTHubDeviceClient_LL_GetPropertiesAndSubscribeToUpdatesAsync(deviceClient, Thermostat_PropertiesCallback, (void*)deviceClient)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set device twin callback, error=%d", iothubResult);
        result = false;
    }
    else
    {
        result = true;
    }

    if (result == false)
    {
        IoTHubDeviceClient_LL_Destroy(deviceClient);
        deviceClient = NULL;
    }

    if ((result == false) &&  (iothubInitResult == 0))
    {
        IoTHub_Deinit();
    }

    return deviceClient;
}

int main(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;

    if (GetConnectionSettingsFromEnvironment(&g_pnpDeviceConfiguration) == false)
    {
        LogError("Cannot read required environment variable(s)");
    }
    else if (BuildUtcTimeFromCurrentTime(g_ProgramStartTime, sizeof(g_ProgramStartTime)) == false)
    {
        LogError("Unable to output the program start time");
    }
    else if ((deviceClient = CreateAndConfigureDeviceClientHandleForPnP()) == NULL)
    {
        LogError("Failed creating IotHub device client");
    }
    else
    {
        LogInfo("Successfully created device client handle.  Hit Control-C to exit program\n");

        int numberOfIterations = 0;
        SendMaxTemperatureSinceReboot(deviceClient);

        while (true)
        {
            // Wake up periodically to poll.  Even if we do not plan on sending telemetry, we still need to poll periodically in order to process
            // incoming requests from the server and to do connection keep alives.
            if ((numberOfIterations % g_sendTelemetryPollInterval) == 0)
            {
                Thermostat_SendCurrentTemperature(deviceClient);
            }

            IoTHubDeviceClient_LL_DoWork(deviceClient);
            ThreadAPI_Sleep(g_sleepBetweenPollsMs);
            numberOfIterations++;
        }

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(deviceClient);
        // Free all the IoT SDK subsystem
        IoTHub_Deinit();        
    }

    return 0;
}
