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
// TODO - testPropName1 => TEST_PROP_NAME1 or following convention now this is a #define
#define testPropName1 "name1"
#define testPropName2 "name2"
#define testPropName3 "name3"
#define testPropValue1 "value1"
#define testPropValue2 "value2"
#define testPropValue3 "value3"

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

    const IOTHUB_CLIENT_REPORTED_PROPERTY testReportedTwoProperties[] = { testReportedProp1, testReportedProp2, testReportedProp3};

    set_expected_calls_for_IoTHubClient_Serialize_ReportedProperties();

    // act
    IOTHUB_CLIENT_RESULT result = IoTHubClient_Serialize_ReportedProperties(testReportedTwoProperties, 3, TEST_COMPONENT_NAME_1, &serializedProperties, &serializedPropertiesLength);

    // assert
    ASSERT_ARE_EQUAL(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, testReportedThreePropertiesComponentJSON, serializedProperties);
    ASSERT_ARE_EQUAL(int, strlen(testReportedThreePropertiesComponentJSON), serializedPropertiesLength);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // free
    IoTHubClient_Serialize_Properties_Destroy(serializedProperties);
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

static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableProp1 = { IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_STRUCT_VERSION_1, testPropName1, testPropValue1, 200, 1, NULL };

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

static const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE testWritableWrongVersion = { 2, testPropName1, testPropValue1, 200, 1, NULL };

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
