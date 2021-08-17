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
#include "RowKeyPredicate.h"

namespace griddb {

Napi::FunctionReference RowKeyPredicate::constructor;

Napi::Object RowKeyPredicate::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "RowKeyPredicate", {
            InstanceMethod("setRange",
                &RowKeyPredicate::setRange),
            InstanceMethod("getRange",
                &RowKeyPredicate::getRange),
            InstanceMethod("setDistinctKeys",
                &RowKeyPredicate::setDistinctKeys),
            InstanceMethod("getDistinctKeys",
                &RowKeyPredicate::getDistinctKeys),
            InstanceAccessor("keyType",
                &RowKeyPredicate::getKeyType, nullptr)
            });
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("RowKeyPredicate", func);
    return exports;
}

RowKeyPredicate::RowKeyPredicate(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<RowKeyPredicate>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 2 || !info[0].IsExternal() || !info[1].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return;
    }
    predicate = info[0].As<Napi::External<GSRowKeyPredicate>>().Data();
    type = (GSType) *(info[1].As<Napi::External<int>>().Data());
}

RowKeyPredicate::~RowKeyPredicate() {
    if (predicate != NULL) {
        gsCloseRowKeyPredicate(&predicate);
        predicate = NULL;
    }
}

Napi::Value RowKeyPredicate::getKeyType(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    return Napi::Number::New(env, type);
}

GSRowKeyPredicate* RowKeyPredicate::getPredicate() {
    return predicate;
}

void RowKeyPredicate::setRange(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (info.Length() != 2) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments\n", NULL)
        return;
    }
    GSResult ret;
    switch (type) {
    case GS_TYPE_INTEGER: {
        if (!info[0].IsNumber() || !info[1].IsNumber()) {
            THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
            return;
        }
        int startKey = info[0].ToNumber();
        int finishKey = info[1].ToNumber();
        ret = gsSetPredicateStartKeyByInteger(predicate,
                (const int32_t *) &startKey);
        if (!GS_SUCCEEDED(ret)) {
            THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
            return;
        }
        ret = gsSetPredicateFinishKeyByInteger(predicate,
                (const int32_t *) &finishKey);
        if (!GS_SUCCEEDED(ret)) {
            THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
            return;
        }
        break;
    }
    case GS_TYPE_STRING:
        if (!info[0].IsString() || !info[1].IsString()) {
            THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
            return;
        }
        ret = gsSetPredicateStartKeyByString(predicate,
                (const char *)info[0].ToString().Utf8Value().c_str());
        if (!GS_SUCCEEDED(ret)) {
            THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
            return;
        }
        ret = gsSetPredicateFinishKeyByString(predicate,
                (const char *)info[1].ToString().Utf8Value().c_str());
        if (!GS_SUCCEEDED(ret)) {
            THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
            return;
        }
        break;
    case GS_TYPE_LONG: {
        if (!info[0].IsNumber() || !info[1].IsNumber()) {
            THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
            return;
        }
        int64_t startKey = info[0].ToNumber();
        int64_t finishKey = info[1].ToNumber();
        ret = gsSetPredicateStartKeyByLong(predicate,
                (const int64_t *) &startKey);
        if (!GS_SUCCEEDED(ret)) {
            THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
            return;
        }
        ret = gsSetPredicateFinishKeyByLong(predicate,
                (const int64_t *) &finishKey);
        if (!GS_SUCCEEDED(ret)) {
            THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
            return;
        }
        break;
    }
    case GS_TYPE_TIMESTAMP: {
        Napi::Value startKey = info[0].As<Napi::Value>();
        Napi::Value finishKey = info[1].As<Napi::Value>();
        GSTimestamp tmpTimestampStart = Util::toGsTimestamp(env, &startKey);
        GSTimestamp tmpTimestampFinish = Util::toGsTimestamp(env, &finishKey);
        ret = gsSetPredicateStartKeyByTimestamp(predicate,
                &tmpTimestampStart);
        if (!GS_SUCCEEDED(ret)) {
            THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
            return;
        }
        ret = gsSetPredicateFinishKeyByTimestamp(predicate,
                &tmpTimestampFinish);
        if (!GS_SUCCEEDED(ret)) {
            THROW_EXCEPTION_WITH_CODE(env, ret, &predicate)
            return;
        }
        break;
    }
    default:
        THROW_EXCEPTION_WITH_STR(env, "Not support type\n", NULL)
        return;
        break;
    }
}

// Get range
Napi::Value RowKeyPredicate::getRange(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (info.Length() != 0) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return env.Null();
    }
    GSValue *startKey = NULL;
    GSValue *endKey = NULL;
    GSResult ret;
    int length = 2;
    Napi::Array array = Napi::Array::New(env, length);
    ret = gsGetPredicateStartKeyGeneral(predicate,
            (const GSValue **) &startKey);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
        return env.Null();
    }
    if (startKey != NULL) {
        switch (type) {
        case GS_TYPE_STRING: {
            array.Set<const GSChar *>("0", startKey->asString);
            break;
        }
        case GS_TYPE_INTEGER: {
            array.Set<int32_t>("0", startKey->asInteger);
            break;
        }
        case GS_TYPE_LONG: {
            array.Set<int64_t>("0", startKey->asLong);
            break;
        }
        case GS_TYPE_TIMESTAMP: {
            array["0"] = Util::fromTimestamp(env,
                static_cast<GSTimestamp *>(&startKey->asTimestamp));
            break;
        }
        default:
            THROW_EXCEPTION_WITH_STR(env, "Not support type\n", NULL)
            return env.Null();
            break;
        }
    }
    ret = gsGetPredicateFinishKeyGeneral(predicate, (const GSValue **)&endKey);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
        return env.Null();
    }
    if (endKey != NULL) {
        switch (type) {
        case GS_TYPE_STRING: {
            array.Set<const GSChar *>("1", endKey->asString);
            break;
        }
        case GS_TYPE_INTEGER: {
            array.Set<int32_t>("1", endKey->asInteger);
            break;
        }
        case GS_TYPE_LONG: {
            array.Set<int64_t>("1", endKey->asLong);
            break;
        }
        case GS_TYPE_TIMESTAMP: {
            array["1"] = Util::fromTimestamp(env,
                static_cast<GSTimestamp *>(&endKey->asTimestamp));
            break;
        }
        default:
            THROW_EXCEPTION_WITH_STR(env, "Not support type\n", NULL)
            return env.Null();
            break;
        }
    }
    return array;
}

void RowKeyPredicate::setDistinctKeys(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments\n", NULL)
        return;
    }
    Napi::Array arrayForSet = info[0].As<Napi::Array>();
    size_t keyCount = arrayForSet.Length();
    GSResult ret;
    for (size_t i = 0; i < keyCount; i++) {
        switch (type) {
        case GS_TYPE_INTEGER: {
            Napi::Value value = arrayForSet[i];
            if (!value.IsNumber()) {
                THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
                return;
            }
            int32_t key = value.ToNumber().Int32Value();
            ret = gsAddPredicateKeyByInteger(predicate, key);
            if (!GS_SUCCEEDED(ret)) {
                THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
                return;
            }
            break;
        }
        case GS_TYPE_STRING: {
            Napi::Value value = arrayForSet[i];
            if (!value.IsString()) {
                THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
                return;
            }
            ret = gsAddPredicateKeyByString(predicate,
                        value.ToString().Utf8Value().c_str());
            if (!GS_SUCCEEDED(ret)) {
                THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
                return;
            }
            break;
        }
        case GS_TYPE_LONG: {
            Napi::Value value = arrayForSet[i];
            if (!value.IsNumber()) {
                THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
                return;
            }
            int64_t key =  value.ToNumber().Int64Value();
            ret = gsAddPredicateKeyByLong(predicate, key);
            if (!GS_SUCCEEDED(ret)) {
                THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
                return;
            }
            break;
        }
        case GS_TYPE_TIMESTAMP: {
            Napi::Value value = arrayForSet[i];
            GSTimestamp key = Util::toGsTimestamp(env, &value);
            ret = gsAddPredicateKeyByTimestamp(predicate, key);
            if (!GS_SUCCEEDED(ret)) {
                THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
                return;
            }
            break;
        }
        default:
            THROW_EXCEPTION_WITH_STR(env, "Not support type\n", NULL)
            return;
            break;
        }
    }
}

Napi::Value RowKeyPredicate::getDistinctKeys(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (info.Length() != 0) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return env.Null();
    }
    size_t size;
    GSValue *keyList;
    GSResult ret;
    ret = gsGetPredicateDistinctKeysGeneral(predicate,
            (const GSValue **)&keyList, &size);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, predicate)
        return env.Null();
    }
    Napi::Array array = Napi::Array::New(env, size);
    for (int i = 0; i < static_cast<int>(size); i++) {
        switch (type) {
        case GS_TYPE_STRING: {
            array.Set<const GSChar *>(i, keyList[i].asString);
            break;
        }
        case GS_TYPE_INTEGER: {
            array.Set<int32_t>(i, keyList[i].asInteger);
            break;
        }
        case GS_TYPE_LONG: {
            array.Set<int64_t>(i, keyList[i].asLong);
            break;
        }
        case GS_TYPE_TIMESTAMP: {
            array[i] = Util::fromTimestamp(env,
                static_cast<GSTimestamp *>(&keyList[i].asTimestamp));
            break;
        }
        default:
            THROW_EXCEPTION_WITH_STR(env, "Not support type\n", NULL)
            return env.Null();
            break;
        }
    }
    return array;
}

}  // namespace griddb
