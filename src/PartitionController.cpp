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
#include "PartitionController.h"

namespace griddb {

#if NAPI_VERSION <= 5
Napi::FunctionReference PartitionController::constructor;
#endif

Napi::Object PartitionController::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "PartitionController",
            { InstanceMethod("getContainerCount",
                    &PartitionController::getContainerCount), InstanceMethod(
                    "getPartitionIndexOfContainer",
                    &PartitionController::getPartitionIndexOfContainer),
                    InstanceAccessor("partitionCount",
                            &PartitionController::getPartitionCount,
                            &PartitionController::setReadonlyAttribute),
                    InstanceMethod("getContainerNames",
                            &PartitionController::getContainerNames)
            });

#if NAPI_VERSION > 5
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    Util::setInstanceData(env, "PartitionController", constructor);
#else
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
#endif
    exports.Set("PartitionController", func);
    return exports;
}

PartitionController::PartitionController(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<PartitionController>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", mController)
        return;
    }

    this->mController =
            info[0].As<Napi::External<GSPartitionController>>().Data();
}

PartitionController::~PartitionController() {
    if (mController != NULL) {
        gsClosePartitionController(&mController);
        mController = NULL;
    }
}

Napi::Value PartitionController::getContainerCount(
        const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    int32_t partition_index = info[0].As<Napi::Number>().Int32Value();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

    int64_t value;
    GSResult ret = gsGetPartitionContainerCount(mController, partition_index,
            &value);

    // Check ret, if error, throw exception
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mController)
    }

    Napi::Value return_wrapper = Napi::Number::New(env, value);
    // Return promise object
    deferred.Resolve(return_wrapper);
    return deferred.Promise();
}

Napi::Value PartitionController::getPartitionIndexOfContainer(
        const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsString()) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(
                deferred, env, "Wrong arguments", mController)
    }
    std::string containerName = info[0].As<Napi::String>().Utf8Value();

    int32_t value;
    GSResult ret = gsGetPartitionIndexOfContainer(
            mController, containerName.c_str(), &value);

    // Check ret, if error, throw exception
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mController)
    }

    Napi::Value returnWrapper = Napi::Number::New(env, value);

    // Return promise object
    deferred.Resolve(returnWrapper);
    return deferred.Promise();
}

Napi::Value PartitionController::getPartitionCount(
        const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    int32_t value;
    GSResult ret = gsGetPartitionCount(mController, &value);

    // Check ret, if error, throw exception
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mController)
        return env.Null();
    }
    return Napi::Number::New(env, value);
}

/**
 *  Throw exception when set value to readonly attribute
 */
void PartitionController::setReadonlyAttribute(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    Napi::Env env = info.Env();
    THROW_EXCEPTION_WITH_STR(env, "Can't set read only attribute", mController)
}

Napi::Value PartitionController::getContainerNames(
        const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 3 || !info[0].IsNumber()|| !info[1].IsNumber()
            || !info[2].IsNumber()) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(
                deferred, env, "Wrong arguments", mController)
    }
    int32_t partition_index = info[0].As<Napi::Number>().Int32Value();
    int64_t start = info[1].As<Napi::Number>().Int64Value();
    int64_t limit = info[2].As<Napi::Number>().Int64Value();
    GSChar **stringList;
    size_t size;

    int64_t *limitPtr;
    if (limit >= 0) {
        limitPtr = &limit;
    } else {
        limitPtr = NULL;
    }
    GSResult ret = gsGetPartitionContainerNames(mController, partition_index,
            start, limitPtr, (const GSChar* const **) &stringList, &size);

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mController)
    }

    Napi::Array return_wrapper = Napi::Array::New(env, size);

    for (int i = 0; i < static_cast<int>(size); i++) {
        return_wrapper.Set(i, Napi::String::New(env, stringList[i]));
    }
    deferred.Resolve(return_wrapper);
    return deferred.Promise();
}

}  // namespace griddb

