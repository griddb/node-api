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

#include "ExpirationInfo.h"
#include "GSException.h"
#include "Macro.h"
#include "Util.h"

namespace griddb {

#if NAPI_VERSION <= 5
Napi::FunctionReference ExpirationInfo::constructor;
#endif

void ExpirationInfo::init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "ExpirationInfo",
            { ExpirationInfo::InstanceAccessor("time", &ExpirationInfo::getTime,
                    &ExpirationInfo::setTime), ExpirationInfo::InstanceAccessor(
                    "unit", &ExpirationInfo::getUnit, &ExpirationInfo::setUnit),
                    ExpirationInfo::InstanceAccessor("divisionCount",
                            &ExpirationInfo::getDivisionCount,
                            &ExpirationInfo::setDivisionCount), });
#if NAPI_VERSION > 5
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    Util::setInstanceData(env, "ExpirationInfo", constructor);
#else
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
#endif
    exports.Set("ExpirationInfo", func);
}

ExpirationInfo::ExpirationInfo(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<ExpirationInfo>(info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    if (info.Length() != 3|| !info[0].IsNumber() || !info[1].IsNumber()
        || !info[2].IsNumber()) {
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return;
    }
    int time = info[0].As<Napi::Number>().Int32Value();
    GSTimeUnit unit = info[1].As<Napi::Number>().Int32Value();
    int division_count = info[2].As<Napi::Number>().Int32Value();

    mTimeSeriesProps.rowExpirationTime = time;
    mTimeSeriesProps.rowExpirationTimeUnit = unit;
    mTimeSeriesProps.expirationDivisionCount = division_count;
    mTimeSeriesProps.compressionList = NULL;
    mTimeSeriesProps.compressionListSize = 0;
    mTimeSeriesProps.compressionMethod = GS_COMPRESSION_NO;
    mTimeSeriesProps.compressionWindowSize = 0;
    mTimeSeriesProps.compressionWindowSizeUnit = GS_TIME_UNIT_YEAR;
}

ExpirationInfo::~ExpirationInfo() {
}

void ExpirationInfo::setTime(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    if (!value.IsNumber()) {
        THROW_EXCEPTION_WITH_STR(info.Env(), "Wrong arguments", NULL)
        return;
    }
    int time = value.As<Napi::Number>().Int32Value();

    mTimeSeriesProps.rowExpirationTime = time;
}

Napi::Value ExpirationInfo::getTime(const Napi::CallbackInfo &info) {
    int time = mTimeSeriesProps.rowExpirationTime;
    return Napi::Number::New(info.Env(), time);
}

void ExpirationInfo::setUnit(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    if (!value.IsNumber()) {
        THROW_EXCEPTION_WITH_STR(info.Env(), "Wrong arguments", NULL)
        return;
    }
    int unit = value.As<Napi::Number>().Int32Value();
    mTimeSeriesProps.rowExpirationTimeUnit = unit;
}

Napi::Value ExpirationInfo::getUnit(const Napi::CallbackInfo &info) {
    int unit = mTimeSeriesProps.rowExpirationTimeUnit;
    return Napi::Number::New(info.Env(), unit);
}

void ExpirationInfo::setDivisionCount(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    if (!value.IsNumber()) {
        THROW_EXCEPTION_WITH_STR(info.Env(), "Wrong arguments", NULL)
        return;
    }
    int division_count = value.As<Napi::Number>().Int32Value();
    mTimeSeriesProps.expirationDivisionCount = division_count;
}

Napi::Value ExpirationInfo::getDivisionCount(const Napi::CallbackInfo &info) {
    int division_count = mTimeSeriesProps.expirationDivisionCount;
    return Napi::Number::New(info.Env(), division_count);
}

GSTimeSeriesProperties* ExpirationInfo::gs_ts() {
    return &mTimeSeriesProps;
}

}  // namespace griddb

