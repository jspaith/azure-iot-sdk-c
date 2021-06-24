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

#include "testrunnerswitcher.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_macro_utils/macro_utils.h"
//#include "azure_c_shared_utility/shared_util_options.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_prod.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

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

#include "azure_c_shared_utility/gballoc.h"

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
    umock_c_negative_tests_deinit();
}

// TODO - move static data out of BEGIN_TEST_SUITE since certain msvc's don't like this.  Keep inline for now for convenience.
// TODO - testPropName1 => TEST_PROP_NAME1 or following convention now this is a #define
#define testPropName1 "name1"
#define testPropName2 "name2"
#define testPropName3 "name3"
#define testPropValue1 "1234"
#define testPropValue2 "\"value2\""
#define testPropValue3 "{\"embeddedJSON\": 123}"

static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedProp1 = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, testPropName1, testPropValue1 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedProp2 = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, testPropName2, testPropValue2 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedProp3 = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, testPropName3, testPropValue3 };

static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedWrongVersion = { 2, testPropName1, testPropValue1 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNULLName = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, NULL, testPropValue1 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNULLValue = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, testPropName1, NULL };

//
// IoTHubClient_Serialize_ReportedProperties tests
//
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
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_NULL_serializedProperties)
{
    // arrange
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(&testReportedProp1, 1, NULL, NULL, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_NULL_serializedPropertiesLength)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(&testReportedProp1, 1, NULL, &serializedProperties, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

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
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_NULL_propname)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNullNameThirdIndex[] = { testReportedProp1, testReportedProp2, testReportedNULLName};
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedNullNameThirdIndex, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_NULL_propvalue)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNullValueThirdIndex[] = { testReportedProp1, testReportedProp2, testReportedNULLValue};
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedNullValueThirdIndex, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

#define BUILD_EXPECTED_JSON(n, v) "\""n"\":"v""

#define testReportedPropertyNoComponentJSONNoBrace BUILD_EXPECTED_JSON(testPropName1, testPropValue1)
#define testReportedPropertyNoComponentJSON "{" testReportedPropertyNoComponentJSONNoBrace "}"
#define testReportedTwoPropertiesNoComponentJSONNoBrace BUILD_EXPECTED_JSON(testPropName1, testPropValue1) "," BUILD_EXPECTED_JSON(testPropName2, testPropValue2)
#define testReportedTwoPropertiesNoComponentJSON "{" testReportedTwoPropertiesNoComponentJSONNoBrace "}"
#define testReportedThreePropertiesNoComponentJSONNoBrace BUILD_EXPECTED_JSON(testPropName1, testPropValue1) "," BUILD_EXPECTED_JSON(testPropName2, testPropValue2) "," BUILD_EXPECTED_JSON(testPropName3, testPropValue3)
#define testReportedThreePropertiesNoComponentJSON  "{" testReportedThreePropertiesNoComponentJSONNoBrace "}"

static const char testReportedProp3NoComponentJSON[] = BUILD_EXPECTED_JSON(testPropName3, testPropValue3);

static void set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties()
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_one_property_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(&testReportedProp1, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testReportedPropertyNoComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testReportedPropertyNoComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_two_properties_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedTwoProperties[] = { testReportedProp1, testReportedProp2};

    set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedTwoProperties, 2, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testReportedTwoPropertiesNoComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testReportedTwoPropertiesNoComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_three_properties_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedTwoProperties[] = { testReportedProp1, testReportedProp2, testReportedProp3};

    set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedTwoProperties, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testReportedThreePropertiesNoComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testReportedThreePropertiesNoComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

#define TEST_COMPONENT_NAME_1 "testComponent"

#define TEST_COMPONENT_MARKER(componentName) "{\""componentName"\":{\"__t\":\"c\""
#define TEST_COMPONENT_JSON(componentName, expectedProperties) TEST_COMPONENT_MARKER(componentName) "," expectedProperties "}"

static const char testReportedPropertyComponentJSON[] = TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, testReportedPropertyNoComponentJSONNoBrace) "}";
static const char testReportedTwoPropertiesComponentJSON[] = TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, testReportedTwoPropertiesNoComponentJSONNoBrace) "}";
static const char testReportedThreePropertiesComponentJSON[] = TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, testReportedThreePropertiesNoComponentJSONNoBrace) "}";

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_one_property_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(&testReportedProp1, 1, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testReportedPropertyComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testReportedPropertyComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_two_properties_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedTwoProperties[] = { testReportedProp1, testReportedProp2};

    set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedTwoProperties, 2, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testReportedTwoPropertiesComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testReportedTwoPropertiesComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_three_properties_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedThreeProperties[] = { testReportedProp1, testReportedProp2, testReportedProp3};

    set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedThreeProperties, 3, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testReportedThreePropertiesComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testReportedThreePropertiesComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_ReportedProperties_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedThreeProperties[] = { testReportedProp1, testReportedProp2, testReportedProp3};

    set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties();
    umock_c_negative_tests_snapshot();

    // act
    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);
            
            IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedThreeProperties, 3, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
            ASSERT_IS_NULL(serializedProperties);
            ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
        }
    }
}


//
// IoTHubClient_Serialize_WritablePropertyResponse tests
// 
static void set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse()
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
}

//
// IoTHubClient_Serialize_ReportedProperties test
//
TEST_FUNCTION(IoTHubClient_Serialize_WritablePropertyResponse_NULL_properties)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(NULL, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

#define TEST_STATUS_CODE_1 200
#define TEST_STATUS_CODE_2 400
#define TEST_STATUS_CODE_3 500

#define TEST_ACK_CODE_1 1
#define TEST_ACK_CODE_2 19
#define TEST_ACK_CODE_3 77

#define TEST_DESCRIPTION_2 "2-description"
#define TEST_DESCRIPTION_3 "3-description"

static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableWrongVersion = { 2, testPropName1, testPropValue1, 
                                                                                   TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL};

static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableProp1 = { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, testPropName1, testPropValue1, 
                                                                   TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL };
static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableProp2 = { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, testPropName2, testPropValue2, 
                                                                   TEST_STATUS_CODE_2, TEST_ACK_CODE_2, TEST_DESCRIPTION_2 };
static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableProp3 = { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, testPropName3, testPropValue3, 
                                                                   TEST_STATUS_CODE_3, TEST_ACK_CODE_3, TEST_DESCRIPTION_3 };

static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableNULLName =  { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, NULL, testPropValue1, 
                                                                      TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL };
static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableNULLValue = { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, testPropName1, NULL,
                                                                      TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL };

TEST_FUNCTION(IoTHubClient_Serialize_WritablePropertyResponse_NULL_serializedProperties)
{
    // arrange
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(&testWritableProp1, 1, NULL, NULL, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Serialize_WritablePropertyResponse_NULL_serializedPropertiesLength)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(&testWritableProp1, 1, NULL, &serializedProperties, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

TEST_FUNCTION(IoTHubClient_Serialize_WritablePropertyResponse_wrong_struct_version)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;
    
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(&testWritableWrongVersion, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(serializedProperties);
    ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// TODO: Add the NULL name/value tests for writable

#define BUILD_EXPECTED_WRITABLE_JSON(name, val, code, version) "\""name"\":{\"value\":"val",\"ac\":" #code ",\"av\":" #version "}"
#define BUILD_EXPECTED_WRITABLE_JSON_DESCRIPTION(name, val, code, version, description) "\""name"\":{\"value\":"val",\"ac\":" #code ",\"av\":" #version ",\"ad\":\"" description "\"}"

#define testWritableProperty1NoComponentJSONNoBrace BUILD_EXPECTED_WRITABLE_JSON(testPropName1, testPropValue1, 200, 1)
#define testWritableProperty1NoComponentJSON "{" testWritableProperty1NoComponentJSONNoBrace "}"

#define testWritableProperty2NoComponentJSONNoBrace BUILD_EXPECTED_WRITABLE_JSON_DESCRIPTION(testPropName2, testPropValue2, 400, 19, TEST_DESCRIPTION_2)
#define testWritableProperty2NoComponentJSON "{" testWritableProperty2NoComponentJSONNoBrace "}"

#define testWritableProperty3NoComponentJSONNoBrace BUILD_EXPECTED_WRITABLE_JSON_DESCRIPTION(testPropName3, testPropValue3, 500, 77, TEST_DESCRIPTION_3)

#define testWritableTwoPropertiesNoComponentJSONNoBrace testWritableProperty1NoComponentJSONNoBrace "," testWritableProperty2NoComponentJSONNoBrace
#define testWritableTwoPropertiesNoComponentJSON "{" testWritableTwoPropertiesNoComponentJSONNoBrace "}"

#define testWritableThreePropertiesNoComponentJSONNoBrace testWritableProperty1NoComponentJSONNoBrace "," testWritableProperty2NoComponentJSONNoBrace "," testWritableProperty3NoComponentJSONNoBrace
#define testWritableThreePropertiesNoComponentJSON "{" testWritableThreePropertiesNoComponentJSONNoBrace "}"


TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_one_property_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(&testWritableProp1, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testWritableProperty1NoComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testWritableProperty1NoComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_one_property_with_description_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(&testWritableProp2, 1, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testWritableProperty2NoComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testWritableProperty2NoComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_two_properties_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableTwoProperties[] = { testWritableProp1, testWritableProp2 };

    set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(testWritableTwoProperties, 2, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testWritableTwoPropertiesNoComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testWritableTwoPropertiesNoComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_three_properties_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableThreeProperties[] = { testWritableProp1, testWritableProp2, testWritableProp3 };

    set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(testWritableThreeProperties, 3, NULL, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testWritableThreePropertiesNoComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testWritableThreePropertiesNoComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

static const char testWritableProperty1ComponentJSON[] = TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, testWritableProperty1NoComponentJSONNoBrace) "}";
static const char testWritableProperty2ComponentJSON[] = TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, testWritableProperty2NoComponentJSONNoBrace) "}";
static const char testWritableTwoPropertiesComponentJSON[] = TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, testWritableTwoPropertiesNoComponentJSONNoBrace) "}";
static const char testWritableThreePropertiesComponentJSON[] = TEST_COMPONENT_JSON(TEST_COMPONENT_NAME_1, testWritableThreePropertiesNoComponentJSONNoBrace) "}";

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_one_property_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(&testWritableProp1, 1, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testWritableProperty1ComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testWritableProperty1ComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_one_property_with_description_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(&testWritableProp2, 1, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testWritableProperty2ComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testWritableProperty2ComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_two_properties_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableTwoProperties[] = { testWritableProp1, testWritableProp2 };

    set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(testWritableTwoProperties, 2, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testWritableTwoPropertiesComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testWritableTwoPropertiesComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_three_properties_with_component_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableThreeProperties[] = { testWritableProp1, testWritableProp2, testWritableProp3 };

    set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(testWritableThreeProperties, 3, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testWritableThreePropertiesComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testWritableThreePropertiesComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
}

TEST_FUNCTION(IoTHubClient_Serialize_WritableProperties_fail)
{
    // arrange
    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableThreeProperties[] = { testWritableProp1, testWritableProp2, testWritableProp3 };

    set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse();
    umock_c_negative_tests_snapshot();

    // act
    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);
            
            IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_WritablePropertyResponse(testWritableThreeProperties, 3, NULL, &serializedProperties, &serializedPropertiesLength);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
            ASSERT_IS_NULL(serializedProperties);
            ASSERT_ARE_EQUAL(int, 0, serializedPropertiesLength);
        }
    }
}


//
// IoTHubClient_Serialize_Properties_Destroy tests
// 
static void set_expected_calls_for_IoTHubClient_Serialize_Properties_Destroy()
{
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

TEST_FUNCTION(IoTHubClient_Serialize_Properties_Destroy_success)
{
    // arrange
    unsigned char* serializedProperties = NULL;
    size_t serializedPropertiesLength = 0;

    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(&testReportedProp1, 1, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(serializedPropertiesLength);

    umock_c_reset_all_calls();
    set_expected_calls_for_IoTHubClient_Serialize_Properties_Destroy();

    // act
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);


    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


TEST_FUNCTION(IoTHubClient_Serialize_Properties_Destroy_null)
{
    // act
    IoTHubClient_Serialize_Properties_Destroy(NULL);

    // Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}





END_TEST_SUITE(iothub_client_properties_ut)
