// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"

#include "azure_macro_utils/macro_utils.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void* my_gballoc_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS
// iothub_client_core_common.h is required under ENABLE_MOCKS so that TEST_DEFINE_ENUM_TYPE
// can be successfully resolved for IOTHUB_CLIENT_RESULT below.
#include "iothub_client_core_common.h"

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "parson.h"

#ifdef __cplusplus
extern "C"
{
#endif
    MOCKABLE_FUNCTION(, JSON_Value*, json_parse_string, const char *, string);
    MOCKABLE_FUNCTION(, char*, json_serialize_to_string, const JSON_Value*, value);
    MOCKABLE_FUNCTION(, void, json_free_serialized_string, char*, string);
    MOCKABLE_FUNCTION(, JSON_Object*, json_value_get_object, const JSON_Value *, value);
    MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);
#ifdef __cplusplus
}
#endif
#undef ENABLE_MOCKS

#include "iothub_client_properties.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    (void)error_code;
    ASSERT_FAIL("umock_c reported error");
}

TEST_DEFINE_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUES);

BEGIN_TEST_SUITE(iothub_client_properties_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    umock_c_init(on_umock_c_error);
    
    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);


    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_calloc, my_gballoc_calloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_realloc, NULL);


    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, real_json_parse_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);

    REGISTER_TYPE(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ;
}


/*
IoTHubClient_Serialize_ReportedProperties(
    const IOTHUB_CLIENT_REPORTED_PROPERTY* properties,
    size_t numProperties,
    const char* componentName,
    unsigned char** serializedProperties,
    size_t* serializedPropertiesLength)

*/
// IoTHubClient_Serialize_ReportedProperties_NULL_properties

#if 0
TEST_FUNCTION(testTemplateForPropertyWriting)
{
    // arrange

    // act
    IOTHUB_CLIENT_RESULT result;

    // assert

    // cleanup
}
#endif

// TODO - move static data out of BEGIN_TEST_SUITE since certain msvc's don't like this.  Keep inline for now for convenience.

static const char testPropName1[] = "name1";
static const char testPropName2[] = "name2";
static const char testPropName3[] = "name3";
static const char testPropValue1[] = "value1";
static const char testPropValue2[] = "value2";
static const char testPropValue3[] = "value3";

static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedWrongVersion = { 2, testPropName1, testPropValue1 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedPropOne = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, testPropName1, testPropValue1 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNULLName = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, NULL, testPropValue1 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNULLValue = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, testPropName1, NULL };


TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_NULL_properties)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(NULL, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_NULL_serializedProperties)
{
    // arrange
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(&testReportedPropOne, 1, NULL, NULL, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_NULL_serializedPropertiesLength)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(&testReportedPropOne, 1, NULL, &serializedProperties, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_wrong_struct_version)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(&testReportedWrongVersion, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_NULL_propname)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNullNameThirdIndex[3] = { testReportedNULLName, testReportedNULLName, testReportedNULLName};
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedNullNameThirdIndex, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_NULL_propvalue)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNullValueThirdIndex[3] = { testReportedNULLValue, testReportedNULLValue, testReportedNULLValue};
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedNullValueThirdIndex, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
}





END_TEST_SUITE(iothub_client_properties_ut)
