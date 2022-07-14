/// @file       parsonWrapper.c
/// @brief      Some useful shortcuts while using the Parson JSON library
/// @author     Birger Hoppe
/// @copyright  (c) 2022 Birger Hoppe
/// @copyright  Permission is hereby granted, free of charge, to any person obtaining a
///             copy of this software and associated documentation files (the "Software"),
///             to deal in the Software without restriction, including without limitation
///             the rights to use, copy, modify, merge, publish, distribute, sublicense,
///             and/or sell copies of the Software, and to permit persons to whom the
///             Software is furnished to do so, subject to the following conditions:\n
///             The above copyright notice and this permission notice shall be included in
///             all copies or substantial portions of the Software.\n
///             THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///             IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///             FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///             AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///             LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///             OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///             THE SOFTWARE.

#include <cstdlib>
#include <cmath>
#include "parsonWrapper.h"

//
// MARK: Object access
//

// tests for 'null', return ptr to value if wanted
bool jog_is_null (const JSON_Object *object, const char *name, JSON_Value** ppValue)
{
    JSON_Value* pJSONVal = json_object_get_value(object, name);
    if (ppValue)
        *ppValue = pJSONVal;
    return !pJSONVal || json_type(pJSONVal) == JSONNull;
}

// access to JSON string fields, with NULL replaced by ""
const char* jog_s (const JSON_Object *object, const char *name)
{
    const char* s = json_object_get_string ( object, name );
    return s ? s : "";
}

// access to JSON number fields, encapsulated as string, with NULL replaced by 0
double jog_sn (const JSON_Object *object, const char *name)
{
    const char* s = json_object_get_string ( object, name );
    return s ? std::strtod(s,NULL) : 0.0;
}

// access to JSON number with 'null' returned as 'NAN'
double jog_n_nan (const JSON_Object *object, const char *name)
{
    JSON_Value* pJSONVal = NULL;
    if (!jog_is_null(object, name, &pJSONVal))
        return json_value_get_number (pJSONVal);
    else
        return NAN;
}

double jog_sn_nan (const JSON_Object *object, const char *name)
{
    const char* s = json_object_get_string ( object, name );
    return s ? strtod(s,NULL) : NAN;
}

//
// MARK: Array access
//

// tests for 'null', return ptr to value if wanted
bool jag_is_null (const JSON_Array *array,
                  size_t idx,
                  JSON_Value** ppValue)
{
    JSON_Value* pJSONVal = json_array_get_value (array, idx);
    if (ppValue)
        *ppValue = pJSONVal;
    return !pJSONVal || json_type(pJSONVal) == JSONNull;
}


// access to JSON array string fields, with NULL replaced by ""
const char* jag_s (const JSON_Array *array, size_t idx)
{
    const char* s = json_array_get_string ( array, idx );
    return s ? s : "";
}

// access to JSON array number fields, encapsulated as string, with NULL replaced by 0
double jag_sn (const JSON_Array *array, size_t idx)
{
    const char* s = json_array_get_string ( array, idx );
    return s ? strtod(s,NULL) : 0.0;
}

// access to JSON array number field with 'null' returned as 'NAN'
double jag_n_nan (const JSON_Array *array, size_t idx)
{
    JSON_Value* pJSONVal = NULL;
    if (!jag_is_null(array, idx, &pJSONVal))
        return json_value_get_number (pJSONVal);
    else
        return NAN;
}
