/*
    Copyright (c) 2020 TOSHIBA Digital Solutions Corporation.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <string>
#include <limits>
#include "Util.h"
#include "Macro.h"
#include "GSException.h"

/**
 * @brief Allocate new memory and setup string data from source string data for the new memory
 * @param *from A pointer stores source string data
 * @return A pointer variable that refers to new string data
 */
const GSChar* Util::strdup(const GSChar *from) {
    if (from == NULL) {
        return NULL;
    }
    GSChar *temp = new char[strlen(from) + 1]();
    strncpy(temp, from, strlen(from));
    return temp;
}

Napi::Value Util::fromRow(const Napi::Env &env, GSRow *row, int columnCount,
        GSType *typeList) {
    Napi::Array ouput = Napi::Array::New(env);
    for (int i = 0; i < columnCount; i++) {
        ouput[i] = Util::fromField(env, row, i, typeList[i]);
    }
    return ouput;
}

static bool isNull(GSRow* row, int32_t rowField) {
    GSBool nullValue;
    GSResult ret;

    ret = gsGetRowFieldNull(row, (int32_t) rowField, &nullValue);
    if (ret != GS_RESULT_OK) {
        return false;
    }
    if (nullValue == GS_TRUE) {
        return true;
    }
    return false;
}

void Util::freeStrData(Napi::Env env, void* data) {
    delete [] reinterpret_cast<GSChar *>(data);
}

static Napi::Value fromFieldAsLong(const Napi::Env& env, GSRow* row,
        int column) {
    int64_t longValue;
    GSResult ret = gsGetRowFieldAsLong(row, (int32_t) column, &longValue);
    ENSURE_SUCCESS(Util::fromFieldAsLong, ret, NULL)
    if (longValue) {
        return Napi::Number::New(env, longValue);
    } else if (isNull(row, column)) {
        // NULL value
        return env.Null();
    } else {
        return Napi::Number::New(env, longValue);
    }
}

static Napi::Value fromFieldAsString(const Napi::Env& env, GSRow* row,
        int column) {
    GSChar *stringValue;
    GSResult ret = gsGetRowFieldAsString(row, (int32_t) column,
            (const GSChar**) &stringValue);
    ENSURE_SUCCESS(Util::fromFieldAsString, ret, NULL)
    if ((stringValue != NULL) && (stringValue[0] == '\0')) {
        // Empty string
        if (isNull(row, column)) {
            // NULL value
            return env.Null();
        } else {
            return Napi::String::New(env, stringValue);
        }
    } else {
        return Napi::String::New(env, stringValue);
    }
}

static Napi::Value fromFieldAsBlob(const Napi::Env& env, GSRow* row,
       int column) {
    GSBlob blobValue;
    GSResult ret = gsGetRowFieldAsBlob(row, (int32_t) column, &blobValue);
    ENSURE_SUCCESS(Util::fromFieldAsBlob, ret, NULL)
    const GSChar* string;
    if (blobValue.size) {
        string = Util::strdup((const GSChar *) blobValue.data);
        return Napi::Buffer<char>::New(env, const_cast<char*>(string),
                blobValue.size, Util::freeStrData);
    } else if (isNull(row, column)) {
        // NULL value
        return env.Null();
    } else {
        string = Util::strdup((const GSChar *) blobValue.data);
        return Napi::Buffer<char>::New(env, const_cast<char*>(string),
                blobValue.size, Util::freeStrData);
    }
}

static Napi::Value fromFieldAsBool(const Napi::Env& env, GSRow* row,
        int column) {
    GSBool boolValue;
    GSResult ret = gsGetRowFieldAsBool(row, (int32_t) column, &boolValue);
    ENSURE_SUCCESS(Util::fromFieldAsBool, ret, NULL)
    if (boolValue) {
        return Napi::Boolean::New(env, static_cast<bool>(boolValue));
    } else if (isNull(row, column)) {
        // NULL value
        return env.Null();
    } else {
        return Napi::Boolean::New(env, static_cast<bool>(boolValue));
    }
}

static Napi::Value fromFieldAsInteger(const Napi::Env& env, GSRow* row,
        int column) {
    int32_t intValue;
    GSResult ret = gsGetRowFieldAsInteger(row, (int32_t) column, &intValue);
    ENSURE_SUCCESS(Util::fromFieldAsInteger, ret, NULL)
    if (intValue) {
        return Napi::Number::New(env, intValue);
    } else if (isNull(row, column)) {
        // NULL value
        return env.Null();
    } else {
        return Napi::Number::New(env, intValue);
    }
}

static Napi::Value fromFieldAsFloat(const Napi::Env& env, GSRow* row,
        int column) {
    float floatValue;
    GSResult ret = gsGetRowFieldAsFloat(row, (int32_t) column, &floatValue);
    ENSURE_SUCCESS(Util::fromFieldAsFloat, ret, NULL)
    if (floatValue) {
        return Napi::Number::New(env, floatValue);
    } else if (isNull(row, column)) {
        // NULL value
        return env.Null();
    } else {
        return Napi::Number::New(env, floatValue);
    }
}

static Napi::Value fromFieldAsDouble(const Napi::Env& env, GSRow* row,
        int column) {
    double doubleValue;
    GSResult ret = gsGetRowFieldAsDouble(row, (int32_t) column, &doubleValue);
    ENSURE_SUCCESS(Util::fromFieldAsDouble, ret, NULL)
    if (doubleValue) {
        return Napi::Number::New(env, doubleValue);
    } else if (isNull(row, column)) {
        // NULL value
        return env.Null();
    } else {
        return Napi::Number::New(env, doubleValue);
    }
}

static Napi::Value fromFieldAsTimestamp(const Napi::Env &env, GSRow *row,
        int column) {
    GSTimestamp timestampValue;
    GSResult ret = gsGetRowFieldAsTimestamp(row, (int32_t) column,
            &timestampValue);
    ENSURE_SUCCESS(Util::fromFieldAsTimestamp, ret, NULL)
    if (timestampValue) {
        return Util::fromTimestamp(env, &timestampValue);
    } else if (isNull(row, column)) {
        // NULL value
        return env.Null();
    } else {
        return Util::fromTimestamp(env, &timestampValue);
    }
}

static Napi::Value fromFieldAsByte(const Napi::Env& env, GSRow* row,
        int column) {
    int8_t byteValue;
    GSResult ret = gsGetRowFieldAsByte(row, (int32_t) column, &byteValue);
    ENSURE_SUCCESS(Util::fromFieldAsInteger, ret, NULL)
    if (byteValue) {
        return Napi::Number::New(env, byteValue);
    } else if (isNull(row, column)) {
        // NULL value
        return env.Null();
    } else {
        return Napi::Number::New(env, byteValue);
    }
}

static Napi::Value fromFieldAsShort(const Napi::Env& env, GSRow* row,
        int column) {
    int16_t shortValue;
    GSResult ret = gsGetRowFieldAsShort(row, (int32_t) column, &shortValue);
    ENSURE_SUCCESS(Util::fromFieldAsShort, ret, NULL)
    if (shortValue) {
        return Napi::Number::New(env, shortValue);
    } else if (isNull(row, column)) {
        // NULL value
        return env.Null();
    } else {
        return Napi::Number::New(env, shortValue);
    }
}

static Napi::Value fromFieldAsGeometry(const Napi::Env& env, GSRow* row,
        int column) {
    GSChar *geoValue;
    GSResult ret = gsGetRowFieldAsGeometry(row, (int32_t) column,
            (const GSChar**) &geoValue);
    ENSURE_SUCCESS(Util::fromFieldAsGeometry, ret, NULL)
    if ((geoValue != NULL) && (geoValue[0] == '\0')) {
        // Empty string
        if (isNull(row, column)) {
            // NULL value
            return env.Null();
        } else {
            return Napi::String::New(env, geoValue);
        }
    } else {
        return Napi::String::New(env, geoValue);
    }
}

Napi::Value Util::fromField(const Napi::Env& env, GSRow* row, int column,
        GSType type) {
    switch (type) {
        case GS_TYPE_LONG:
            return fromFieldAsLong(env, row, column);
            break;
        case GS_TYPE_STRING:
            return fromFieldAsString(env, row, column);
            break;
        case GS_TYPE_BLOB:
            return fromFieldAsBlob(env, row, column);
            break;
        case GS_TYPE_BOOL:
            return fromFieldAsBool(env, row, column);
            break;
        case GS_TYPE_INTEGER:
            return fromFieldAsInteger(env, row, column);
            break;
        case GS_TYPE_FLOAT:
            return fromFieldAsFloat(env, row, column);
            break;
        case GS_TYPE_DOUBLE:
            return fromFieldAsDouble(env, row, column);
            break;
        case GS_TYPE_TIMESTAMP:
            return fromFieldAsTimestamp(env, row, column);
            break;
        case GS_TYPE_BYTE:
            return fromFieldAsByte(env, row, column);
            break;
        case GS_TYPE_SHORT:
            return fromFieldAsShort(env, row, column);
            break;
        case GS_TYPE_GEOMETRY:
            return fromFieldAsGeometry(env, row, column);
            break;
        default:
            THROW_EXCEPTION_WITH_STR(env, "Type is not support.", NULL)
        }
    return env.Null();
}

Napi::Value Util::fromTimestamp(const Napi::Env& env, GSTimestamp *timestamp) {
#if (NAPI_VERSION > 4)
    return Napi::Date::New(env, *timestamp);
#else
    return Napi::Number::New(env, *timestamp);
#endif
}

static void toFieldAsString(const Napi::Env &env, Napi::Value *value,
        GSRow *row, int column) {
    if (!value->IsString()) {
        THROW_CPP_EXCEPTION_WITH_STR(env, "Input error")
        return;
    }
    std::string stringVal;
    stringVal = value->As<Napi::String>().Utf8Value();
    GSResult ret = gsSetRowFieldByString(row, column, stringVal.c_str());
    ENSURE_SUCCESS_CPP(Util::toFieldAsString, ret)
}

static void toFieldAsLong(const Napi::Env &env, Napi::Value *value, GSRow *row,
            int column) {
    if (!value->IsNumber()) {
        THROW_CPP_EXCEPTION_WITH_STR(env, "Input error")
    }

    // input can be integer
    int64_t longVal = value->As<Napi::Number>().Int64Value();
    // When input value is integer,
    // it should be between -9007199254740992(-2^53)/9007199254740992(2^53).
    if (!(MIN_LONG <= longVal && MAX_LONG >= longVal)) {
        THROW_EXCEPTION_WITH_STR(env, "Input error", NULL)
        return;
    }

    GSResult ret = gsSetRowFieldByLong(row, column, longVal);
    ENSURE_SUCCESS_CPP(Util::toFieldAsLong, ret)
}

static void toFieldAsBool(const Napi::Env &env, Napi::Value *value, GSRow *row,
        int column) {
    bool boolVal;
    if (value->IsBoolean()) {
        boolVal = value->As<Napi::Boolean>().Value();
    } else {
        boolVal = value->ToBoolean().Value();
    }

    GSResult ret = gsSetRowFieldByBool(row, column, boolVal);
    ENSURE_SUCCESS_CPP(Util::toFieldAsBool, ret)
}

static void toFieldAsByte(const Napi::Env &env, Napi::Value *value, GSRow *row,
        int column) {
    if (!value->IsNumber()) {
        THROW_CPP_EXCEPTION_WITH_STR(env, "Input error")
        return;
    }
    int tmpInt = value->As<Napi::Number>().Int32Value();
    if (tmpInt < std::numeric_limits < int8_t > ::min()
            || tmpInt > std::numeric_limits < int8_t > ::max()) {
        THROW_CPP_EXCEPTION_WITH_STR(env, "Input error")
        return;
    }
    GSResult ret = gsSetRowFieldByByte(row, column, tmpInt);
    ENSURE_SUCCESS_CPP(Util::toFieldAsByte, ret)
}

static void toFieldAsShort(const Napi::Env &env, Napi::Value *value, GSRow *row,
        int column) {
    if (!value->IsNumber()) {
        THROW_CPP_EXCEPTION_WITH_STR(env, "Input error")
        return;
    }
    int tmpInt = value->As<Napi::Number>().Int32Value();
    if (tmpInt < std::numeric_limits < int16_t > ::min()
            || tmpInt > std::numeric_limits < int16_t > ::max()) {
        THROW_CPP_EXCEPTION_WITH_STR(env, "Input error")
        return;
    }
    GSResult ret = gsSetRowFieldByShort(row, column, tmpInt);
    ENSURE_SUCCESS_CPP(Util::toFieldAsShort, ret)
}

static void toFieldAsInteger(const Napi::Env &env, Napi::Value *value,
        GSRow *row, int column) {
    if (!value->IsNumber()) {
        THROW_CPP_EXCEPTION_WITH_STR(env, "Input error")
        return;
    }
    int tmpInt = value->As<Napi::Number>().Int32Value();
    GSResult ret = gsSetRowFieldByInteger(row, column, tmpInt);
    ENSURE_SUCCESS_CPP(Util::toFieldAsInteger, ret)
}

static void toFieldAsFloat(const Napi::Env &env, Napi::Value *value, GSRow *row,
        int column) {
    if (!value->IsNumber()) {
        THROW_CPP_EXCEPTION_WITH_STR(env, "Input error")
        return;
    }
    float floatVal = value->As<Napi::Number>().FloatValue();

    GSResult ret = gsSetRowFieldByFloat(row, column, floatVal);
    ENSURE_SUCCESS_CPP(Util::toFieldAsFloat, ret)
}

static void toFieldAsDouble(const Napi::Env &env, Napi::Value *value,
        GSRow *row, int column) {
    if (!value->IsNumber()) {
        Napi::Object gsException = griddb::GSException::New(env, "Input error");
        THROW_GSEXCEPTION(gsException)
        return;
    }
    double doubleVal = value->As<Napi::Number>().DoubleValue();
    GSResult ret = gsSetRowFieldByDouble(row, column, doubleVal);
    ENSURE_SUCCESS_CPP(Util::toFieldAsDouble, ret)
}

GSTimestamp Util::toGsTimestamp(const Napi::Env &env, Napi::Value *value) {
    GSTimestamp timestampVal;

#if (NAPI_VERSION > 4)
    if (value->IsDate()) {
        timestampVal = value->As<Napi::Date>().ValueOf();
        return timestampVal;
    }
#endif
    if (value->IsString()) {
        std::string datetimestring =
                value->As<Napi::String>().ToString().Utf8Value();

        GSBool ret = gsParseTime(
                (const GSChar *)datetimestring.c_str(), &timestampVal);
        if (ret != GS_TRUE) {
            Napi::Object gsException = griddb::GSException::New(
                    env, "Invalid datetime string");
            THROW_GSEXCEPTION(gsException)
            return 0;
        }
    } else if (value->IsNumber()) {
        timestampVal = value->As<Napi::Number>().Int64Value();
        if (timestampVal > (UTC_TIMESTAMP_MAX * 1000)) {  // miliseconds
            Napi::Object gsException = griddb::GSException::New(
                    env, "Invalid timestamp");
            THROW_GSEXCEPTION(gsException)
            return 0;
        }
    } else {
        // Invalid input
        Napi::Object gsException = griddb::GSException::New(
                env, "Invalid input");
        THROW_GSEXCEPTION(gsException)
        return 0;
    }
    return timestampVal;
}

static void toFieldAsTimestamp(const Napi::Env &env, Napi::Value *value,
        GSRow *row, int column) {
    GSTimestamp timestampVal = Util::toGsTimestamp(env, value);
    GSBool ret = gsSetRowFieldByTimestamp(row, column, timestampVal);
    ENSURE_SUCCESS_CPP(Util::toFieldAsTimestamp, ret)
}

static void toFieldAsBlob(const Napi::Env &env, Napi::Value *value, GSRow *row,
        int column) {
    GSBlob blobVal;
    Napi::Buffer<char> stringBuffer = value->As<Napi::Buffer<char>>();
    char *v = static_cast<char*>(stringBuffer.Data());
    size_t size = stringBuffer.Length();
    blobVal.data = v;
    blobVal.size = size;
    GSBool ret = gsSetRowFieldByBlob(row, column, (const GSBlob*) &blobVal);
    ENSURE_SUCCESS_CPP(Util::toFieldAsBlob, ret)
}

void Util::toField(const Napi::Env &env, Napi::Value *value, GSRow *row,
        int column, GSType type) {
    switch (type) {
    case GS_TYPE_STRING: {
        toFieldAsString(env, value, row, column);
        break;
    }
    case GS_TYPE_LONG: {
        toFieldAsLong(env, value, row, column);
        break;
    }
    case GS_TYPE_BOOL: {
        toFieldAsBool(env, value, row, column);
        break;
    }
    case GS_TYPE_BYTE: {
        toFieldAsByte(env, value, row, column);
        break;
    }

    case GS_TYPE_SHORT: {
        toFieldAsShort(env, value, row, column);
        break;
    }
    case GS_TYPE_INTEGER: {
        toFieldAsInteger(env, value, row, column);
        break;
    }
    case GS_TYPE_FLOAT: {
        toFieldAsFloat(env, value, row, column);
        break;
    }
    case GS_TYPE_DOUBLE: {
        toFieldAsDouble(env, value, row, column);
        break;
    }

    case GS_TYPE_TIMESTAMP: {
        toFieldAsTimestamp(env, value, row, column);
        break;
    }

    case GS_TYPE_BLOB: {
        toFieldAsBlob(env, value, row, column);
        break;
    }
    default:
        Napi::Object gsException = griddb::GSException::New(
                env, "Output error.");
        THROW_GSEXCEPTION(gsException)
        break;
    }
}

