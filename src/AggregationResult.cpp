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

#include "AggregationResult.h"
#include "Macro.h"

namespace griddb {

Napi::FunctionReference AggregationResult::constructor;

Napi::Object AggregationResult::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "AggregationResult", {
            InstanceMethod("get", &AggregationResult::get) });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("AggregationResult", func);
    return exports;
}

AggregationResult::AggregationResult(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<AggregationResult>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", mAggResult)
        return;
    }
    this->mAggResult = info[0].As<Napi::External<GSAggregationResult>>().Data();
}

AggregationResult::~AggregationResult() {
    if (mAggResult != NULL) {
        gsCloseAggregationResult(&mAggResult);
    }
    mAggResult = NULL;
}

Napi::Value AggregationResult::get(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    GSType type;
    if (info.Length() != 1 || !info[0].IsNumber()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", mAggResult)
        return env.Null();
    }
    type = info[0].As<Napi::Number>().Int32Value();

    void *value = NULL;
    if (!(type == GS_TYPE_DOUBLE || type == GS_TYPE_LONG
            || type == GS_TYPE_TIMESTAMP)) {
        THROW_EXCEPTION_WITH_STR(env,
            "Not support type from Aggregation result", mAggResult)
        return env.Null();
    }

    double tmpDouble;
    int64_t tmpLong;
    GSTimestamp tmpTimestamp;
    switch (type) {
        case GS_TYPE_DOUBLE: {
            value = &tmpDouble;
            break;
        }
        case GS_TYPE_LONG:
            value = &tmpLong;
            break;
        case GS_TYPE_TIMESTAMP:
            value = &tmpTimestamp;
            break;
        default:
            break;
        }
    GSBool ret = gsGetAggregationValue(mAggResult, value, type);
    if (ret == GS_FALSE) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mAggResult)
        return env.Null();
    }

    switch (type) {
    case GS_TYPE_DOUBLE: {
        return Napi::Number::New(env, tmpDouble);
        break;
    }
    case GS_TYPE_LONG:
        return Napi::Number::New(env, tmpLong);
        break;
    case GS_TYPE_TIMESTAMP:
        return Util::fromTimestamp(env, &tmpTimestamp);
        break;
    default:
        THROW_EXCEPTION_WITH_STR(env, "Error type", mAggResult)
        return env.Null();
        break;
    }
}

}  // namespace griddb
