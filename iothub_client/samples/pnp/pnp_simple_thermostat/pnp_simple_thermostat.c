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

// Environment variable used to specify how app connects to hub and the two possible values
static const char g_securityTypeEnvironmentVariable[] = "IOTHUB_DEVICE_SECURITY_TYPE";
static const char g_securityTypeConnectionStringValue[] = "connectionString";
static const char g_securityTypeDpsValue[] = "DPS";

// Environment variable used to specify this application's connection string
static const char g_connectionStringEnvironmentVariable[] = "IOTHUB_DEVICE_CONNECTION_STRING";

#ifdef USE_PROV_MODULE_FULL
// Environment variable used to specify this application's DPS id scope
static const char g_dpsIdScopeEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_ID_SCOPE";

// Environment variable used to specify this application's DPS device id
static const char g_dpsDeviceIdEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_DEVICE_ID";

// Environment variable used to specify this application's DPS device key
static const char g_dpsDeviceKeyEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_DEVICE_KEY";

// Environment variable used to optionally specify this application's DPS id scope
static const char g_dpsEndpointEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_ENDPOINT";

// Global provisioning endpoint for DPS if one is not specified via the environment
static const char g_dps_DefaultGlobalProvUri[] = "global.azure-devices-provisioning.net";

// Values of connection / security settings read from environment variables and/or DPS runtime
PNP_DEVICE_CONFIGURATION g_pnpDeviceConfiguration;
#endif // USE_PROV_MODULE_FULL

// Connection string used to authenticate device when connection strings are used
const char* g_pnpDeviceConnectionString;

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
// about the setting of the temperature.  Because we accept all temperature requests, we 
// always indicate a success.  An actual sensor could optionally return information about
// a temperature being out of range or a mechanical issue on the device on error cases.
static const char g_temperaturePropertyResponseDescription[] = "success";

// Size of buffer to store ISO 8601 time.
#define TIME_BUFFER_SIZE 128
// Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
static const char g_ISO8601Format[] = "%Y-%m-%dT%H:%M:%SZ";
// Start time of the program, stored in ISO 8601 format string for UTC.
char g_ProgramStartTime[TIME_BUFFER_SIZE];

//
// CopyTwinPayloadToString takes the twin payload data, which arrives as a potentially non-NULL terminated string, and creates
// a new copy of the data with a NULL terminator.  The JSON parser this sample uses, parson, only operates over NULL terminated strings.
//
static char* CopyTwinPayloadToString(const unsigned char* payload, size_t size)
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
    else if ((jsonStr = CopyTwinPayloadToString(payload, size)) == NULL)
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
// SendTargetTemperatureReport sends a PnP property indicating the device has received the desired targeted temperature
//
static void SendTargetTemperatureReport(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClientLL, double desiredTemp, int responseStatus, int version, const char* description)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    char targetTemperatureAsString[32]; // TODO - no magic #'s.

    if (snprintf(targetTemperatureAsString, sizeof(targetTemperatureAsString), g_targetTemperaturePropertyResponseFormat, desiredTemp) != 0)
    {
        LogError("Unable to create target temperature string for reporting result");
    }
    else
    {
        // IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE in this case specifies the response to a desired temperature.
        IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE temperatureProperty;
        memset(&temperatureProperty, 0, sizeof(temperatureProperty));
        // Specify the structure version (not to be confused with the $version on IoT Hub) to protect back-compat in case the structure adds fields.
        temperatureProperty.structVersion= IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_VERSION_1;
        // This represents the version of the request from IoT Hub.  It needs to be returned so service applications can determine
        // what current version of the writable property the device is currently using, as the server may update the property even when the device
        // is offline.
        temperatureProperty.ackVersion = version;
        // Result of request, which maps to HTTP status code.  For sample we'll always indicate success.
        temperatureProperty.result = responseStatus;
        temperatureProperty.name = g_targetTemperaturePropertyName;
        temperatureProperty.value = targetTemperatureAsString;
        temperatureProperty.description = description;

        unsigned char* propertySerialized;
        size_t propertySerializedLength;

        // The first step of reporting properties is to serialize it into an IoT Hub friendly format.  You can do this by either
        // implementing the PnP convention for building up the correct JSON or more simply to use IoTHubClient_Serialize_WritablePropertyResponse.
        if ((iothubClientResult = IoTHubClient_Serialize_WritablePropertyResponse(&temperatureProperty, 1, NULL, &propertySerialized, &propertySerializedLength)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to serialize updated property, error=%d", iothubClientResult);
        }
        // The output of IoTHubClient_Serialize_WritablePropertyResponse is sent to IoTHubDeviceClient_LL_SendPropertiesAsync to perform network I/O.
        else if ((iothubClientResult = IoTHubDeviceClient_LL_SendPropertiesAsync(deviceClientLL, propertySerialized, propertySerializedLength, NULL, NULL)) != IOTHUB_CLIENT_OK)
        {
            LogError("Unable to send updated property, error=%d", iothubClientResult);
        }
        else
        {
            LogInfo("Sending acknowledgement of property to IoTHub");
        }
        IoTHubClient_Serialize_Properties_Destroy(propertySerialized);
    }
}

//
// SendMaxTemperatureSinceReboot reports a PnP property indicating the maximum temperature since the last reboot (simulated here by lifetime of executable)
//
static void SendMaxTemperatureSinceReboot(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClientLL)
{
    IOTHUB_CLIENT_RESULT iothubClientResult;
    char maximumTemperatureAsString[32];

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
        else if ((iothubClientResult = IoTHubDeviceClient_LL_SendPropertiesAsync(deviceClientLL, propertySerialized, propertySerializedLength,  NULL, NULL)) != IOTHUB_CLIENT_OK)
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
static void Thermostat_ProcessTargetTemperature(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClientLL, IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property, int propertiesVersion)
{
    char* next;
    double targetTemperature = strtol(property->value.str, &next, 10);
    if ((property->value.str == next) || (targetTemperature == LONG_MAX) || (targetTemperature == LONG_MIN))
    {
        LogError("Property %s is not a valid integer", property->value.str);
    }
    else
    {
        LogInfo("Received targetTemperature = %f", targetTemperature);
        
        bool maxTempUpdated = false;
        UpdateTemperatureAndStatistics(targetTemperature, &maxTempUpdated);
        
        // The device needs to let the service know that it has received the targetTemperature desired property.
        SendTargetTemperatureReport(deviceClientLL, targetTemperature, g_statusSuccess, propertiesVersion, g_temperaturePropertyResponseDescription);
        
        if (maxTempUpdated)
        {
            // If the Maximum temperature has been updated, we also report this as a property.
            SendMaxTemperatureSinceReboot(deviceClientLL);
        }
    }

}

//
// Thermostat_UpdatedPropertyCallback is invoked by the IoT SDK when a twin - either full twin or a PATCH update - arrives.
//
static void Thermostat_UpdatedPropertyCallback(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,  const unsigned char* payload, size_t payloadLength, void* userContextCallback)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClientLL = (IOTHUB_DEVICE_CLIENT_LL_HANDLE)userContextCallback;
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE propertyIterator;
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY property;
    int propertiesVersion;
    IOTHUB_CLIENT_RESULT clientResult;

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
                // We are iterating over a property that the device has previously sent to IoT Hub; 
                // this shows what IoT Hub has recorded the reported property as.
                //
                // There are scenarios where a device may use this, such as knowing whether the
                // given property has changed on the device and needs to be re-reported.
                //
                // This sample doesn't do anything with this, so we'll continue on when we hit reported properties.
                continue;
            }
            
            if (property.componentName != NULL) 
            {   
                LogError("Property=%s arrived for a non-root component, which we do not support", property.name);
            }
            else if (strcmp(property.componentName, g_targetTemperaturePropertyName) == 0)
            {
                Thermostat_ProcessTargetTemperature(deviceClientLL, &property, propertiesVersion);
            }
            else
            {
                LogError("Property=%s is not implemented", property.name);
            }
            
            IoTHubClient_Deserialize_Properties_DestroyProperty(&property);
        }
    }

    IoTHubClient_Deserialize_Properties_DestroyIterator(propertyIterator);
}

//
// Thermostat_SendCurrentTemperature sends a PnP telemetry indicating the current temperature
//
void Thermostat_SendCurrentTemperature(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClientLL) 
{
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;

    char temperatureStringBuffer[32];

    if (snprintf(temperatureStringBuffer, sizeof(temperatureStringBuffer), g_temperatureTelemetryBodyFormat, g_currentTemperature) < 0)
    {
        LogError("snprintf of current temperature telemetry failed");
    }
    else if ((messageHandle = IoTHubMessage_CreateFromString(temperatureStringBuffer)) == NULL)
    {
        LogError("IoTHubMessage_CreateFromString failed");
    }
    else if ((iothubResult = IoTHubDeviceClient_LL_SendTelemetryAsync(deviceClientLL, messageHandle, NULL, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to send telemetry message, error=%d", iothubResult);
    }

    IoTHubMessage_Destroy(messageHandle);
}

//
// GetConnectionStringFromEnvironment retrieves the connection string based on environment variable
//
static bool GetConnectionStringFromEnvironment()
{
    bool result;

    if ((g_pnpDeviceConnectionString = getenv(g_connectionStringEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_connectionStringEnvironmentVariable);
        result = false;
    }
    else
    {
#ifdef USE_PROV_MODULE_FULL
        g_pnpDeviceConfiguration.securityType = PNP_CONNECTION_SECURITY_TYPE_CONNECTION_STRING;
#endif
        result = true;    
    }

    return result;
}

//
// GetDpsFromEnvironment retrieves DPS configuration for a symmetric key based connection
// from environment variables
//
static bool GetDpsFromEnvironment()
{
#ifndef USE_PROV_MODULE_FULL
    // Explain to user misconfiguration.  The "run_e2e_tests" must be set to OFF because otherwise
    // the e2e's test HSM layer and symmetric key logic will conflict.
    LogError("DPS based authentication was requested via environment variables, but DPS is not enabled.");
    LogError("DPS is an optional component of the Azure IoT C SDK.  It is enabled with symmetric keys at cmake time by");
    LogError("passing <-Duse_prov_client=ON -Dhsm_type_symm_key=ON -Drun_e2e_tests=OFF> to cmake's command line");
    return false;
#else
    bool result;

    if ((g_pnpDeviceConfiguration.u.dpsConnectionAuth.endpoint = getenv(g_dpsEndpointEnvironmentVariable)) == NULL)
    {
        // We will fall back to standard endpoint if one is not specified
        g_pnpDeviceConfiguration.u.dpsConnectionAuth.endpoint = g_dps_DefaultGlobalProvUri;
    }

    if ((g_pnpDeviceConfiguration.u.dpsConnectionAuth.idScope = getenv(g_dpsIdScopeEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_dpsIdScopeEnvironmentVariable);
        result = false;
    }
    else if ((g_pnpDeviceConfiguration.u.dpsConnectionAuth.deviceId = getenv(g_dpsDeviceIdEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_dpsDeviceIdEnvironmentVariable);
        result = false;
    }
    else if ((g_pnpDeviceConfiguration.u.dpsConnectionAuth.deviceKey = getenv(g_dpsDeviceKeyEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_dpsDeviceKeyEnvironmentVariable);
        result = false;
    }
    else
    {
        g_pnpDeviceConfiguration.securityType = PNP_CONNECTION_SECURITY_TYPE_DPS;
        result = true;    
    }

    return result;
#endif // USE_PROV_MODULE_FULL
}


//
// GetConfigurationFromEnvironment reads how to connect to the IoT Hub (using 
// either a connection string or a DPS symmetric key) from the environment.
//
static bool GetConnectionSettingsFromEnvironment()
{
    const char* securityTypeString;
    bool result;

    if ((securityTypeString = getenv(g_securityTypeEnvironmentVariable)) == NULL)
    {
        LogError("Cannot read environment variable=%s", g_securityTypeEnvironmentVariable);
        result = false;
    }
    else
    {
        if (strcmp(securityTypeString, g_securityTypeConnectionStringValue) == 0)
        {
            result = GetConnectionStringFromEnvironment();
        }
        else if (strcmp(securityTypeString, g_securityTypeDpsValue) == 0)
        {
            result = GetDpsFromEnvironment();
        }
        else
        {
            LogError("Environment variable %s must be either %s or %s", g_securityTypeEnvironmentVariable, g_securityTypeConnectionStringValue, g_securityTypeDpsValue);
            result = false;
        }
    }

    return result;    
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
    return IoTHubDeviceClient_LL_CreateFromConnectionString(g_pnpDeviceConnectionString, MQTT_Protocol);
}

//
// CreateAndConfigureDeviceClientHandleForPnP creates a IOTHUB_DEVICE_CLIENT_LL_HANDLE for this application, setting its
// ModelId along with various callbacks.
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE CreateAndConfigureDeviceClientHandleForPnP(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceHandle = NULL;
    IOTHUB_CLIENT_RESULT iothubResult;
    bool urlAutoEncodeDecode = true;
    int iothubInitResult;
    bool result;

    // Before invoking ANY IoTHub Device SDK functionality, IoTHub_Init must be invoked.
    if ((iothubInitResult = IoTHub_Init()) != 0)
    {
        LogError("Failure to initialize client.  Error=%d", iothubInitResult);
        result = false;
    }
    // Create the deviceHandle itself.
    else if ((deviceHandle = CreateDeviceClientLLHandle()) == NULL)
    {
        LogError("Failure creating IotHub client.  Hint: Check your connection string or DPS configuration");
        result = false;
    }
    // Sets verbosity level
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_LOG_TRACE, &g_hubClientTraceEnabled)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set logging option, error=%d", iothubResult);
        result = false;
    }
    // Sets the name of ModelId for this PnP device.
    // This *MUST* be set before the client is connected to IoTHub.  We do not automatically connect when the 
    // handle is created, but will implicitly connect to subscribe for command and property callbacks below.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_MODEL_ID, g_ThermostatModelId)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the ModelID, error=%d", iothubResult);
        result = false;
    }
    // Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set auto Url encode option, error=%d", iothubResult);
        result = false;
    }
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on systems without built in certificate stores.
    else if ((iothubResult = IoTHubDeviceClient_LL_SetOption(deviceHandle, OPTION_TRUSTED_CERT, certificates)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set the trusted cert, error=%d", iothubResult);
        result = false;
    }
#endif // SET_TRUSTED_CERT_IN_SAMPLES
    // Sets the callback function that processes incoming commands, which is the channel PnP Commands are transferred over
    else if ((iothubResult = IoTHubDeviceClient_LL_SubscribeToCommands(deviceHandle, Thermostat_CommandCallback, NULL)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to subscribe for commands, error=%d", iothubResult);
        result = false;
    }
    // Sets the callback function that processes device twin changes from the IoTHub, which is the channel that PnP Properties are 
    // transferred over.  This will also automatically retrieve the full twin for the application. 
    else if ((iothubResult = IoTHubDeviceClient_LL_GetPropertiesAndSubscribeToUpdatesAsync(deviceHandle, Thermostat_UpdatedPropertyCallback, (void*)deviceHandle)) != IOTHUB_CLIENT_OK)
    {
        LogError("Unable to set device twin callback, error=%d", iothubResult);
        result = false;
    }
    else
    {
        result = true;
    }

    if ((result == false) && (deviceHandle != NULL))
    {
        IoTHubDeviceClient_LL_Destroy(deviceHandle);
        deviceHandle = NULL;
    }

    if ((result == false) &&  (iothubInitResult == 0))
    {
        IoTHub_Deinit();
    }

    return deviceHandle;
}

int main(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClientLL = NULL;

    if (GetConnectionSettingsFromEnvironment() == false)
    {
        LogError("Cannot read required environment variable(s)");
    }
    else if (BuildUtcTimeFromCurrentTime(g_ProgramStartTime, sizeof(g_ProgramStartTime)) == false)
    {
        LogError("Unable to output the program start time");
    }
    else if ((deviceClientLL = CreateAndConfigureDeviceClientHandleForPnP()) == NULL)
    {
        LogError("Failed creating IotHub device client");
    }
    else
    {
        LogInfo("Successfully created device client handle.  Hit Control-C to exit program\n");

        int numberOfIterations = 0;
        SendMaxTemperatureSinceReboot(deviceClientLL);

        while (true)
        {
            // Wake up periodically to poll.  Even if we do not plan on sending telemetry, we still need to poll periodically in order to process
            // incoming requests from the server and to do connection keep alives.
            if ((numberOfIterations % g_sendTelemetryPollInterval) == 0)
            {
                Thermostat_SendCurrentTemperature(deviceClientLL);
            }

            IoTHubDeviceClient_LL_DoWork(deviceClientLL);
            ThreadAPI_Sleep(g_sleepBetweenPollsMs);
            numberOfIterations++;
        }

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(deviceClientLL);
        // Free all the IoT SDK subsystem
        IoTHub_Deinit();        
    }

    return 0;
}
