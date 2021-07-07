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
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_prod.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS
// iothub_client_core_common.h is required under ENABLE_MOCKS so that TEST_DEFINE_ENUM_TYPE
// can be successfully resolved for IOTHUB_CLIENT_RESULT below.
//#include "iothub_client_core_common.h"

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
    MOCKABLE_FUNCTION(, JSON_Value*, json_object_get_value, const JSON_Object *, object, const char *, name);
    MOCKABLE_FUNCTION(, void, json_value_free, JSON_Value*, value);
    MOCKABLE_FUNCTION(, JSON_Value_Type, json_value_get_type, const JSON_Value *, value);
    MOCKABLE_FUNCTION(, JSON_Object*, json_object_get_object, const JSON_Object *, object, const char*, name);
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

    REGISTER_GLOBAL_MOCK_HOOK(json_parse_string, real_json_parse_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_parse_string, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(json_serialize_to_string, real_json_serialize_to_string);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_serialize_to_string, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(json_free_serialized_string, real_json_free_serialized_string);

    REGISTER_GLOBAL_MOCK_HOOK(json_value_get_object, real_json_value_get_object );
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_object, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_value, real_json_object_get_value );
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_value, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(json_value_free, real_json_value_free);

    REGISTER_GLOBAL_MOCK_HOOK(json_value_get_type, real_json_value_get_type);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_value_get_type, JSONError);

    REGISTER_GLOBAL_MOCK_HOOK(json_object_get_object, real_json_object_get_object);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(json_object_get_object, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mallocAndStrcpy_s, real_mallocAndStrcpy_s);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mallocAndStrcpy_s, MU_FAILURE);

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
#define TEST_PROP_NAME1 "name1"
#define TEST_PROP_NAME2 "name2"
#define TEST_PROP_NAME3 "name3"
#define TEST_PROP_NAME4 "name4"
#define TEST_PROP_NAME5 "name5"
#define TEST_PROP_NAME6 "name6"
#define TEST_PROP_VALUE1 "1234"
#define TEST_PROP_VALUE2 "\"value2\""
#define TEST_PROP_VALUE3 "{\"embeddedJSON\":123}"
#define TEST_PROP_VALUE4 "4321"
#define TEST_PROP_VALUE5 "\"value5\""
#define TEST_PROP_VALUE6 "{\"embeddedJSON\":321}"

#define TEST_PROP_VALUE1_LEN (sizeof(TEST_PROP_VALUE1) / sizeof(TEST_PROP_VALUE1[0]) - 1)
#define TEST_PROP_VALUE2_LEN (sizeof(TEST_PROP_VALUE2) / sizeof(TEST_PROP_VALUE2[0]) - 1)
#define TEST_PROP_VALUE3_LEN (sizeof(TEST_PROP_VALUE3) / sizeof(TEST_PROP_VALUE3[0]) - 1)
#define TEST_PROP_VALUE4_LEN (sizeof(TEST_PROP_VALUE4) / sizeof(TEST_PROP_VALUE4[0]) - 1)
#define TEST_PROP_VALUE5_LEN (sizeof(TEST_PROP_VALUE5) / sizeof(TEST_PROP_VALUE5[0]) - 1)
#define TEST_PROP_VALUE6_LEN (sizeof(TEST_PROP_VALUE6) / sizeof(TEST_PROP_VALUE6[0]) - 1)


static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedProp1 = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, TEST_PROP_NAME1, TEST_PROP_VALUE1 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedProp2 = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, TEST_PROP_NAME2, TEST_PROP_VALUE2 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedProp3 = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, TEST_PROP_NAME3, TEST_PROP_VALUE3 };

static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedWrongVersion = { 2, TEST_PROP_NAME1, TEST_PROP_VALUE1 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNULLName = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, NULL, TEST_PROP_VALUE1 };
static const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedNULLValue = { IOTHUB_CLIENT_REPORTED_PROPERTY_STRUCT_VERSION_1, TEST_PROP_NAME1, NULL };

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

#define testReportedPropertyNoComponentJSONNoBrace BUILD_EXPECTED_JSON(TEST_PROP_NAME1, TEST_PROP_VALUE1)
#define testReportedPropertyNoComponentJSON "{" testReportedPropertyNoComponentJSONNoBrace "}"
#define testReportedTwoPropertiesNoComponentJSONNoBrace BUILD_EXPECTED_JSON(TEST_PROP_NAME1, TEST_PROP_VALUE1) "," BUILD_EXPECTED_JSON(TEST_PROP_NAME2, TEST_PROP_VALUE2)
#define testReportedTwoPropertiesNoComponentJSON "{" testReportedTwoPropertiesNoComponentJSONNoBrace "}"
#define testReportedThreePropertiesNoComponentJSONNoBrace BUILD_EXPECTED_JSON(TEST_PROP_NAME1, TEST_PROP_VALUE1) "," BUILD_EXPECTED_JSON(TEST_PROP_NAME2, TEST_PROP_VALUE2) "," BUILD_EXPECTED_JSON(TEST_PROP_NAME3, TEST_PROP_VALUE3)
#define testReportedThreePropertiesNoComponentJSON  "{" testReportedThreePropertiesNoComponentJSONNoBrace "}"

static const char testReportedProp3NoComponentJSON[] = BUILD_EXPECTED_JSON(TEST_PROP_NAME3, TEST_PROP_VALUE3);

static void set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties(void)
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

#define TEST_COMPONENT_NAME_1 "testComponent1"
#define TEST_COMPONENT_NAME_2 "testComponent2"
#define TEST_COMPONENT_NAME_3 "testComponent3"

// TODO: These #define string builders need more commentary.
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
static void set_expected_calls_for_IoTHubClient_Serialize_WritablePropertyResponse(void)
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

static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableWrongVersion = { 2, TEST_PROP_NAME1, TEST_PROP_VALUE1, 
                                                                                   TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL};

static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableProp1 = { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, TEST_PROP_NAME1, TEST_PROP_VALUE1, 
                                                                   TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL };
static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableProp2 = { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, TEST_PROP_NAME2, TEST_PROP_VALUE2, 
                                                                   TEST_STATUS_CODE_2, TEST_ACK_CODE_2, TEST_DESCRIPTION_2 };
static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableProp3 = { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, TEST_PROP_NAME3, TEST_PROP_VALUE3, 
                                                                   TEST_STATUS_CODE_3, TEST_ACK_CODE_3, TEST_DESCRIPTION_3 };

static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableNULLName =  { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, NULL, TEST_PROP_VALUE1, 
                                                                      TEST_STATUS_CODE_1, TEST_ACK_CODE_1, NULL };
static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableNULLValue = { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, TEST_PROP_NAME1, NULL,
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

#define testWritableProperty1NoComponentJSONNoBrace BUILD_EXPECTED_WRITABLE_JSON(TEST_PROP_NAME1, TEST_PROP_VALUE1, 200, 1)
#define testWritableProperty1NoComponentJSON "{" testWritableProperty1NoComponentJSONNoBrace "}"

#define testWritableProperty2NoComponentJSONNoBrace BUILD_EXPECTED_WRITABLE_JSON_DESCRIPTION(TEST_PROP_NAME2, TEST_PROP_VALUE2, 400, 19, TEST_DESCRIPTION_2)
#define testWritableProperty2NoComponentJSON "{" testWritableProperty2NoComponentJSONNoBrace "}"

#define testWritableProperty3NoComponentJSONNoBrace BUILD_EXPECTED_WRITABLE_JSON_DESCRIPTION(TEST_PROP_NAME3, TEST_PROP_VALUE3, 500, 77, TEST_DESCRIPTION_3)

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
static void set_expected_calls_for_IoTHubClient_Serialize_Properties_Destroy(void)
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

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


TEST_FUNCTION(IoTHubClient_Serialize_Properties_Destroy_null)
{
    // act
    IoTHubClient_Serialize_Properties_Destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// IoTHubClient_Deserialize_Properties_CreateIterator tests
//
static void set_expected_calls_for_IoTHubClient_Deserialize_Properties_CreateIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, size_t numComponents)
{
    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));

    if (numComponents != 0)
    {
        STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
        for (size_t i = 0; i < numComponents; i++)
        {
            STRICT_EXPECTED_CALL(mallocAndStrcpy_s(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
        }
    }

    STRICT_EXPECTED_CALL(gballoc_calloc(IGNORED_NUM_ARG, IGNORED_NUM_ARG));
    STRICT_EXPECTED_CALL(json_parse_string(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_object(IGNORED_PTR_ARG));
    if (payloadType == IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL)
    {
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();
        STRICT_EXPECTED_CALL(json_object_get_object(IGNORED_PTR_ARG, IGNORED_PTR_ARG)).CallCannotFail();;
    }
    STRICT_EXPECTED_CALL(json_object_get_value(IGNORED_PTR_ARG, IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(json_value_get_type(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}


#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define TEST_TWIN_VER_1 17
#define TEST_TWIN_VER_2  1010

#define TEST_JSON_TWIN_VER_1  "\"$version\":" STR(TEST_TWIN_VER_1)
#define TEST_JSON_TWIN_VER_2  "\"$version\":" STR(TEST_TWIN_VER_2)

#define TEST_JSON_NAME_VALUE1 "\""  TEST_PROP_NAME1 "\":" TEST_PROP_VALUE1
#define TEST_JSON_NAME_VALUE2 "\""  TEST_PROP_NAME2 "\":" TEST_PROP_VALUE2
#define TEST_JSON_NAME_VALUE3 "\""  TEST_PROP_NAME3 "\":" TEST_PROP_VALUE3
#define TEST_JSON_NAME_VALUE4 "\""  TEST_PROP_NAME4 "\":" TEST_PROP_VALUE4
#define TEST_JSON_NAME_VALUE5 "\""  TEST_PROP_NAME5 "\":" TEST_PROP_VALUE5
#define TEST_JSON_NAME_VALUE6 "\""  TEST_PROP_NAME6 "\":" TEST_PROP_VALUE6

static unsigned const char TEST_JSON_ONE_PROPERTY_ALL[] = "{ \"desired\": { " TEST_JSON_NAME_VALUE1 "," TEST_JSON_TWIN_VER_1 "} }";
static const size_t TEST_JSON_ONE_PROPERTY_ALL_LEN = sizeof(TEST_JSON_ONE_PROPERTY_ALL) - 1;
static unsigned const char TEST_JSON_ONE_PROPERTY_WRITABLE[] = "{" TEST_JSON_NAME_VALUE1 "," TEST_JSON_TWIN_VER_2 "}";
static const size_t TEST_JSON_ONE_PROPERTY_WRITABLE_LEN = sizeof(TEST_JSON_ONE_PROPERTY_WRITABLE) - 1;

static unsigned const char TEST_JSON_TWO_PROPERTIES_ALL[] = "{ \"desired\": { " TEST_JSON_NAME_VALUE1 "," TEST_JSON_NAME_VALUE2 "," TEST_JSON_TWIN_VER_1 "} }";
static const size_t TEST_JSON_TWO_PROPERTIES_ALL_LEN = sizeof(TEST_JSON_TWO_PROPERTIES_ALL) - 1;
static unsigned const char TEST_JSON_TWO_PROPERTIES_WRITABLE[] = "{" TEST_JSON_NAME_VALUE1 "," TEST_JSON_NAME_VALUE2 "," TEST_JSON_TWIN_VER_2 "}";
static const size_t TEST_JSON_TWO_PROPERTIES_WRITABLE_LEN = sizeof(TEST_JSON_TWO_PROPERTIES_WRITABLE) - 1;

static unsigned const char TEST_JSON_THREE_PROPERTIES_ALL[] = "{ \"desired\": { " TEST_JSON_NAME_VALUE1 "," TEST_JSON_NAME_VALUE2 "," TEST_JSON_NAME_VALUE3 "," TEST_JSON_TWIN_VER_1 "} }";
static const size_t TEST_JSON_THREE_PROPERTIES_ALL_LEN = sizeof(TEST_JSON_THREE_PROPERTIES_ALL) - 1;
static unsigned const char TEST_JSON_THREE_PROPERTIES_WRITABLE[] = "{" TEST_JSON_NAME_VALUE1 "," TEST_JSON_NAME_VALUE2 "," TEST_JSON_NAME_VALUE3 "," TEST_JSON_TWIN_VER_2 "}";
static const size_t TEST_JSON_THREE_PROPERTIES_WRITABLE_LEN = sizeof(TEST_JSON_THREE_PROPERTIES_WRITABLE) - 1;

static unsigned const char TEST_JSON_ONE_REPORTED_PROPERTY_ALL[] = "{ \"reported\": {" TEST_JSON_NAME_VALUE4 "},  \"desired\": { " TEST_JSON_TWIN_VER_1 "} }";
static const size_t TEST_JSON_ONE_REPORTED_PROPERTY_ALL_LEN = sizeof(TEST_JSON_ONE_REPORTED_PROPERTY_ALL) - 1;

static unsigned const char TEST_JSON_TWO_REPORTED_PROPERTIES_ALL[] = "{ \"reported\": {" TEST_JSON_NAME_VALUE4 "," TEST_JSON_NAME_VALUE5 "},  \"desired\": { " TEST_JSON_TWIN_VER_1 "} }";
static const size_t TEST_JSON_TWO_REPORTED_PROPERTIES_ALL_LEN = sizeof(TEST_JSON_TWO_REPORTED_PROPERTIES_ALL) - 1;

static unsigned const char TEST_JSON_THREE_REPORTED_PROPERTIES_ALL[] = "{ \"reported\": {" TEST_JSON_NAME_VALUE4 "," TEST_JSON_NAME_VALUE5 "," TEST_JSON_NAME_VALUE6 "},  \"desired\": { " TEST_JSON_TWIN_VER_1 "} }";
static const size_t TEST_JSON_THREE_REPORTED_PROPERTIES_ALL_LEN = sizeof(TEST_JSON_THREE_REPORTED_PROPERTIES_ALL) - 1;

static unsigned const char TEST_JSON_ONE_REPORTED_UPDATE_PROPERTY_ALL[] = "{ \"reported\": {" TEST_JSON_NAME_VALUE4 "},  \"desired\": { " TEST_JSON_NAME_VALUE1 "," TEST_JSON_TWIN_VER_1 "} }";
static const size_t TEST_JSON_ONE_REPORTED_UPDATE_PROPERTY_ALL_LEN = sizeof(TEST_JSON_ONE_REPORTED_UPDATE_PROPERTY_ALL) - 1;

static unsigned const char TEST_JSON_TWO_REPORTED_UPDATE_PROPERTIES_ALL[] = "{ \"reported\": {" TEST_JSON_NAME_VALUE4 "," TEST_JSON_NAME_VALUE5 "},  \"desired\": { " TEST_JSON_NAME_VALUE1 "," TEST_JSON_NAME_VALUE2 "," TEST_JSON_TWIN_VER_1 "} }";
static const size_t TEST_JSON_TWO_REPORTED_UPDATE_PROPERTIES_ALL_LEN = sizeof(TEST_JSON_TWO_REPORTED_UPDATE_PROPERTIES_ALL) - 1;

static unsigned const char TEST_JSON_THREE_REPORTED_UPDATE_PROPERTIES_ALL[] = "{ \"reported\": {" TEST_JSON_NAME_VALUE4 "," TEST_JSON_NAME_VALUE5 "," TEST_JSON_NAME_VALUE6 "},  \"desired\": { " TEST_JSON_NAME_VALUE1 "," TEST_JSON_NAME_VALUE2 "," TEST_JSON_NAME_VALUE3 "," TEST_JSON_TWIN_VER_1 "} }";
static const size_t TEST_JSON_THREE_REPORTED_UPDATE_PROPERTIES_ALL_LEN = sizeof(TEST_JSON_THREE_REPORTED_UPDATE_PROPERTIES_ALL) - 1;



static unsigned const char TEST_INVALID_JSON[] = "}{-not-valid";
static const size_t TEST_INVALID_JSON_LEN = sizeof(TEST_INVALID_JSON) - 1;

// Legal JSON including $version, but for an ALL json its missing the desired.  Creating iterator will be OK with this won't have anything to enumerate
static unsigned const char TEST_JSON_NO_DESIRED[] = "{ " TEST_JSON_TWIN_VER_1 " }";
static const size_t TEST_JSON_NO_DESIRED_LEN = sizeof(TEST_JSON_NO_DESIRED) - 1;

// Legal JSON but no $Version.  Creating iterator won't allow this.
static unsigned const char TEST_JSON_NO_VERSION[] = "44";
static const size_t TEST_JSON_NO_VERSION_LEN = sizeof(TEST_INVALID_JSON) - 1;

static IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, const unsigned char* payload, 
                                                                          size_t payloadLength, const char** componentsInModel, size_t numComponentsInModel)
{
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = NULL;
    set_expected_calls_for_IoTHubClient_Deserialize_Properties_CreateIterator(payloadType, numComponentsInModel);

    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_CreateIterator(payloadType, payload, payloadLength, componentsInModel, numComponentsInModel, &h);

    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_reset_all_calls();

    return h;
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_invalid_payload_type)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = NULL;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_CreateIterator((IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE)1234, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0, &h);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_NULL_payload)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = NULL;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_CreateIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, NULL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0, &h);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_zero_payloadLength)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = NULL;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_CreateIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, 0, NULL, 0, &h);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_NULL(h);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_NULL_handle)
{
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_CreateIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_all_success)
{
    // act
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

static const char* TEST_COMPONENT_LIST[] = {TEST_COMPONENT_NAME_1, TEST_COMPONENT_NAME_2, TEST_COMPONENT_NAME_3};
static const size_t TEST_COMPONENT_LIST_LEN = sizeof(TEST_COMPONENT_LIST) / sizeof(TEST_COMPONENT_LIST[0]);

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_all_with_components_success)
{
    // act
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, 
                                                                            TEST_COMPONENT_LIST, TEST_COMPONENT_LIST_LEN);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_writable_success)
{
    // act
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE, TEST_JSON_ONE_PROPERTY_WRITABLE_LEN, 
                                                                            NULL, 0);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_no_properties_success)
{
    // act
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_NO_DESIRED, TEST_JSON_NO_DESIRED_LEN, 
                                                                            NULL, 0);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_writable_with_components_success)
{
    // act
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE, TEST_JSON_ONE_PROPERTY_WRITABLE_LEN, 
                                                                            TEST_COMPONENT_LIST, TEST_COMPONENT_LIST_LEN);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

static void test_IoTHubClient_Deserialize_Properties_CreateIterator_invalid_json(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, const unsigned char* invalidJson, size_t invalidJsonLen)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = NULL;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_CreateIterator(payloadType, invalidJson, invalidJsonLen, NULL, 0, &h);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_ERROR, result);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_writable_invalid_JSON_fail)
{
    test_IoTHubClient_Deserialize_Properties_CreateIterator_invalid_json(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_INVALID_JSON, TEST_INVALID_JSON_LEN);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_writable_missing_version_fail)
{
    test_IoTHubClient_Deserialize_Properties_CreateIterator_invalid_json(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_NO_VERSION, TEST_JSON_NO_VERSION_LEN);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_CreateIterator_fail)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = NULL;

    int negativeTestsInitResult = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, negativeTestsInitResult);

    set_expected_calls_for_IoTHubClient_Deserialize_Properties_CreateIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_COMPONENT_LIST_LEN);
    umock_c_negative_tests_snapshot();

    // act
    size_t count = umock_c_negative_tests_call_count();
    for (size_t index = 0; index < count; index++)
    {
        if (umock_c_negative_tests_can_call_fail(index))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(index);

            IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_CreateIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, 
                                                                                             TEST_COMPONENT_LIST, TEST_COMPONENT_LIST_LEN, &h);

            //assert
            ASSERT_ARE_NOT_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
            ASSERT_IS_NULL(h);
        }
    }
}

//
// IoTHubClient_Deserialize_Properties_GetVersion tests
//

// TODO: Make sure an int is correct size?  Should it be 64 bit instead??
static const int TEST_DEFAULT_PROPERTIES_VERSION = 119;

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetVersion_NULL_handle)
{
    // arrange
    int propertiesVersion = TEST_DEFAULT_PROPERTIES_VERSION;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetVersion(NULL, &propertiesVersion);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    // Make sure test didn't inadvertently change value of properties version on failure
    ASSERT_ARE_EQUAL(int, TEST_DEFAULT_PROPERTIES_VERSION, propertiesVersion);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetVersion_NULL_version)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE, TEST_JSON_ONE_PROPERTY_WRITABLE_LEN, 
                                                                            TEST_COMPONENT_LIST, TEST_COMPONENT_LIST_LEN);
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetVersion(h, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);    
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetVersion_writable_update_success)
{
    // arrange
    int propertiesVersion;
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE, TEST_JSON_ONE_PROPERTY_WRITABLE_LEN, 
                                                                            TEST_COMPONENT_LIST, TEST_COMPONENT_LIST_LEN);
    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetVersion(h, &propertiesVersion);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, TEST_TWIN_VER_2, propertiesVersion);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetVersion_full_twin_success)
{
    // arrange
    int propertiesVersion;
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetVersion(h, &propertiesVersion);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(int, TEST_TWIN_VER_1, propertiesVersion);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

//
// IoTHubClient_Deserialize_Properties_GetNextProperty tests
//
// TODO: not consistent casing between like ResetTestProperty && set_expected_...
static void ResetTestProperty(IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property)
{
    memset(property, 0, sizeof(*property));
    property->structVersion = IOTHUB_CLIENT_DESERIALIZED_PROPERTY_STRUCT_VERSION_1;
}

static void CompareProperties(const IOTHUB_CLIENT_DESERIALIZED_PROPERTY* expectedProperty, IOTHUB_CLIENT_DESERIALIZED_PROPERTY* actualProperty)
{
    ASSERT_ARE_EQUAL(int, expectedProperty->structVersion, actualProperty->structVersion);
    ASSERT_ARE_EQUAL(int, expectedProperty->propertyType, actualProperty->propertyType);
    ASSERT_ARE_EQUAL(char_ptr, expectedProperty->componentName, actualProperty->componentName);
    ASSERT_ARE_EQUAL(char_ptr, expectedProperty->name, actualProperty->name);
    ASSERT_ARE_EQUAL(int, expectedProperty->valueType, actualProperty->valueType);
    ASSERT_ARE_EQUAL(char_ptr, expectedProperty->value.str, actualProperty->value.str);
    ASSERT_ARE_EQUAL(int, expectedProperty->valueLength, actualProperty->valueLength);
}

// For error cases, make sure IoTHubClient_Deserialize_Properties_GetNextProperty does not change any members
// of the passed in structure
static IOTHUB_CLIENT_DESERIALIZED_PROPERTY unfilledProperty = {
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY_STRUCT_VERSION_1,
    0,
    NULL,
    NULL,
    0,
    { .str = NULL },
    0
};

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_NULL_handle)
{
    // arrange
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY property;
    ResetTestProperty(&property);
    bool propertySpecified = false;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetNextProperty(NULL, &property, &propertySpecified);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_FALSE(propertySpecified);
    CompareProperties(&unfilledProperty, &property);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_NULL_property)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0);
    bool propertySpecified = false;

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetNextProperty(h, NULL, &propertySpecified);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    ASSERT_IS_FALSE(propertySpecified);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_NULL_propertySpecified)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0);
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY property;
    ResetTestProperty(&property);

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetNextProperty(h, &property, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    CompareProperties(&unfilledProperty, &property);

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_wrong_struct_version)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0);
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY wrongVersion;
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY property;

    memset(&wrongVersion, 0, sizeof(wrongVersion));
    wrongVersion.structVersion = IOTHUB_CLIENT_DESERIALIZED_PROPERTY_STRUCT_VERSION_1;
    memcpy(&property, &wrongVersion, sizeof(property));

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetNextProperty(h, &property, NULL);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_INVALID_ARG, result);
    // Make sure IoTHubClient_Deserialize_Properties_GetNextProperty didn't modify property in error case
    ASSERT_IS_TRUE(0 == memcmp(&wrongVersion, &property, sizeof(property)));

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

static IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedProperty1 = {
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE,
    NULL,
    TEST_PROP_NAME1,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE1 },
    TEST_PROP_VALUE1_LEN
};

static IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedProperty2 = {
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE,
    NULL,
    TEST_PROP_NAME2,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE2 },
    TEST_PROP_VALUE2_LEN
};

static IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedProperty3 = {
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE,
    NULL,
    TEST_PROP_NAME3,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE3 },
    TEST_PROP_VALUE3_LEN
};

static IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedProperty4 = {
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE,
    NULL,
    TEST_PROP_NAME4,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE4 },
    TEST_PROP_VALUE4_LEN
};

static IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedProperty5 = {
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE,
    NULL,
    TEST_PROP_NAME5,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE5 },
    TEST_PROP_VALUE5_LEN
};

static IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedProperty6 = {
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY_STRUCT_VERSION_1,
    IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE,
    NULL,
    TEST_PROP_NAME6,
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    { .str = TEST_PROP_VALUE6 },
    TEST_PROP_VALUE6_LEN
};


static void TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, const unsigned char* payload, size_t payloadLength, const char** componentsInModel, size_t numComponentsInModel,
                                       const IOTHUB_CLIENT_DESERIALIZED_PROPERTY* expectedProperties, size_t numExpectedProperties)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(payloadType, payload, payloadLength, componentsInModel, numComponentsInModel);
    size_t numPropertiesVisited = 0;

    // act|assert

    while (true)
    {
        IOTHUB_CLIENT_DESERIALIZED_PROPERTY property;
        bool propertySpecified;
        ResetTestProperty(&property);

        IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetNextProperty(h, &property, &propertySpecified);
        ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);

        if (numPropertiesVisited == numExpectedProperties)
        {
            // Last time through enumeration loop we should be donee.
            ASSERT_IS_FALSE(propertySpecified);
            break;
        }
        ASSERT_IS_TRUE(propertySpecified);

        CompareProperties(expectedProperties + numPropertiesVisited, &property);

        IoTHubClient_Deserialize_Properties_DestroyProperty(&property);
        numPropertiesVisited++;
    }

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);
}

// Test only to just skip STRICT_EXPECT work
TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_all_one_property_success)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty1 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0, expectedPropList, 1);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_writable_one_property_success)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty1 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_ONE_PROPERTY_WRITABLE, TEST_JSON_ONE_PROPERTY_WRITABLE_LEN, NULL, 0, expectedPropList, 1);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_no_properties_success)
{
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_NO_DESIRED, TEST_JSON_NO_DESIRED_LEN, NULL, 0, NULL, 0);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_all_two_properties)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty1, expectedProperty2 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_PROPERTIES_ALL, TEST_JSON_TWO_PROPERTIES_ALL_LEN, NULL, 0, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_writable_two_properties)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty1, expectedProperty2 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_TWO_PROPERTIES_WRITABLE, TEST_JSON_TWO_PROPERTIES_WRITABLE_LEN, NULL, 0, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_all_three_properties)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty1, expectedProperty2, expectedProperty3 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_PROPERTIES_ALL, TEST_JSON_THREE_PROPERTIES_ALL_LEN, NULL, 0, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_writable_three_properties)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty1, expectedProperty2, expectedProperty3 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_WRITABLE_UPDATES, TEST_JSON_THREE_PROPERTIES_WRITABLE, TEST_JSON_THREE_PROPERTIES_WRITABLE_LEN, NULL, 0, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_reported_one_property)
{
     IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty4 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_REPORTED_PROPERTY_ALL, TEST_JSON_ONE_REPORTED_PROPERTY_ALL_LEN, NULL, 0, expectedPropList, 1);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_reported_two_properties)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty4, expectedProperty5 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_REPORTED_PROPERTIES_ALL, TEST_JSON_TWO_REPORTED_PROPERTIES_ALL_LEN, NULL, 0, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_reported_three_properties)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty4, expectedProperty5, expectedProperty6 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_REPORTED_PROPERTIES_ALL, TEST_JSON_THREE_REPORTED_PROPERTIES_ALL_LEN, NULL, 0, expectedPropList, 3);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_one_reported_update_properties)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty1, expectedProperty4  };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_REPORTED_UPDATE_PROPERTY_ALL, TEST_JSON_ONE_REPORTED_UPDATE_PROPERTY_ALL_LEN, NULL, 0, expectedPropList, 2);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_two_reported_update_properties)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty1, expectedProperty2, expectedProperty4, expectedProperty5 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_TWO_REPORTED_UPDATE_PROPERTIES_ALL, TEST_JSON_TWO_REPORTED_UPDATE_PROPERTIES_ALL_LEN, NULL, 0, expectedPropList, 4);
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_GetNextProperty_three_reported_update_properties)
{
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY expectedPropList[] = { expectedProperty1, expectedProperty2, expectedProperty3, expectedProperty4, expectedProperty5, expectedProperty6 };
    TestDeserializedProperties(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_THREE_REPORTED_UPDATE_PROPERTIES_ALL, TEST_JSON_THREE_REPORTED_UPDATE_PROPERTIES_ALL_LEN, NULL, 0, expectedPropList, 6);
}


// Next round of tests.
// One reported, one desired at same time.  Then two.  And then three.  Fortunately again only PAYLOAD_ALL
// And then... inside a component, writable.  Basically 1/2/3 in single component.  And then 3 in separate components.  And then reported combos.
// Finally the "ultimate" test- which failure test should be based - 3 reported/3 desired props, each in separate component.
//  For _fail() run, just show let call recorder run and see what it says and make sure reasonable and use that instead of a manual re-derive.

//
// IoTHubClient_Deserialize_Properties_DestroyProperty tests
//
static void set_expected_calls_for_IoTHubClient_Deserialize_Properties_DestroyProperty(void)
{
    STRICT_EXPECTED_CALL(json_free_serialized_string(IGNORED_PTR_ARG));
}


TEST_FUNCTION(IoTHubClient_Deserialize_Properties_DestroyProperty_ok)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0);
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY property;
    bool propertySpecified;
    ResetTestProperty(&property);

    IOTHUB_CLIENT_RESULT result = IoTHubClient_Deserialize_Properties_GetNextProperty(h, &property, &propertySpecified);
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_IS_TRUE(propertySpecified);
    umock_c_reset_all_calls();

    set_expected_calls_for_IoTHubClient_Deserialize_Properties_DestroyProperty();

    // act
    IoTHubClient_Deserialize_Properties_DestroyProperty(&property);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);

}


TEST_FUNCTION(IoTHubClient_Deserialize_Properties_DestroyProperty_null)
{
    // act
    IoTHubClient_Deserialize_Properties_DestroyProperty(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// IoTHubClient_Deserialize_Properties_DestroyIterator tests
//
static void set_expected_calls_for_IoTHubClient_Deserialize_Properties_DestroyIterator(size_t numComponentsInModel)
{
    if (numComponentsInModel != 0)
    {
        for (size_t i = 0; i < numComponentsInModel; i++)
        {
            STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
        }
        STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
    }
    STRICT_EXPECTED_CALL(json_value_free(IGNORED_PTR_ARG));
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_DestroyIterator_success)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, NULL, 0);
    set_expected_calls_for_IoTHubClient_Deserialize_Properties_DestroyIterator(0);

    // act
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(IoTHubClient_Deserialize_Properties_DestroyIterator_multiple_components_success)
{
    // arrange
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE h = TestAllocatePropertyIterator(IOTHUB_CLIENT_PROPERTY_PAYLOAD_ALL, TEST_JSON_ONE_PROPERTY_ALL, TEST_JSON_ONE_PROPERTY_ALL_LEN, 
                                                                            TEST_COMPONENT_LIST, TEST_COMPONENT_LIST_LEN);
    set_expected_calls_for_IoTHubClient_Deserialize_Properties_DestroyIterator(TEST_COMPONENT_LIST_LEN);

    // act
    IoTHubClient_Deserialize_Properties_DestroyIterator(h);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


TEST_FUNCTION(IoTHubClient_Deserialize_Properties_DestroyIterator_null)
{
    // act
    IoTHubClient_Deserialize_Properties_DestroyIterator(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(iothub_client_properties_ut)
