/// @file       parsonWrapper.h
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

#pragma once

#include "parson/parson.h"

//
// MARK: Object access
//

/// tests object for 'null', returns ptr to value if wanted
bool jog_is_null (const JSON_Object *object,
                  const char *name,
                  JSON_Value** ppValue = NULL);

/// access to JSON string fields, with NULL replaced by ""
const char* jog_s (const JSON_Object *object, const char *name);

/// access to JSON floating number fields, encapsulated as string, with NULL replaced by 0
double jog_sn (const JSON_Object *object, const char *name);

/// access to JSON number fields, encapsulated as string, with NULL replaced by 0
inline long jog_sl (const JSON_Object *object, const char *name)
{
    return std::lround(jog_sn (object, name));
}

/// access to JSON floating number field (just a shorter name, returns 0 if not a number)
inline double jog_n (const JSON_Object *object, const char *name)
{
    return json_object_get_number (object, name);
}

/// access to JSON number field (just a shorter name, returns 0 if not a number)
inline long jog_l (const JSON_Object *object, const char *name)
{
    return std::lround(json_object_get_number (object, name));
}

/// access to JSON number with 'null' returned as 'NAN'
double jog_n_nan (const JSON_Object *object, const char *name);

/// access to JSON number encapsulated as string with 'null' returned as 'NAN'
double jog_sn_nan (const JSON_Object *object, const char *name);

/// access to JSON boolean field (replaces -1 with false)
inline bool jog_b (const JSON_Object *object, const char *name)
{
    // json_object_get_boolean returns -1 if field doesn't exit, so we
    // 'convert' -1 and 0 both to false with the following comparison:
    return json_object_get_boolean (object, name) > 0;
}

/// interprets a string-encapsulated number "0" as false, all else as true
inline bool jog_sb (const JSON_Object *object, const char *name)
{
    return jog_sl (object, name) != 0;
}

//
// MARK: Array access
//

/// tests array for 'null', returns ptr to value if wanted
bool jag_is_null (const JSON_Array *array,
                  size_t idx,
                  JSON_Value** ppValue = NULL);

/// access to JSON array string fields, with NULL replaced by ""
const char* jag_s (const JSON_Array *array, size_t idx);

/// access to JSON array number fields, encapsulated as string, with NULL replaced by 0
double jag_sn (const JSON_Array *array, size_t idx);

/// access to JSON array number field (just a shorter name, returns 0 if not number)
inline double jag_n (const JSON_Array *array, size_t idx)
{
    return json_array_get_number (array, idx);
}

/// access to JSON array number field with `null` returned as `NAN`
double jag_n_nan (const JSON_Array *array, size_t idx);

/// access to JSON array boolean field (replaces -1 with false)
inline bool jag_b (const JSON_Array *array, size_t idx)
{
    // json_object_get_boolean returns -1 if field doesn't exit, so we
    // 'convert' -1 and 0 both to false with the following comparison:
    return json_array_get_boolean (array, idx) > 0;
}
