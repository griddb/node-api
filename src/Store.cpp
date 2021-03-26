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
#include "Store.h"

namespace griddb {

Napi::FunctionReference Store::constructor;

Napi::Object Store::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "Store", {
            InstanceMethod("putContainer",
                &Store::putContainer),
            InstanceMethod(
                "dropContainer", &Store::dropContainer),
            InstanceMethod(
                "getContainer", &Store::getContainer),
            InstanceMethod(
                "getContainerInfo", &Store::getContainerInfo),
            InstanceAccessor("partitionController",
                &Store::getPartitionController,
                &Store::setReadonlyAttribute)
            });
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Store", func);
    return exports;
}

Store::Store(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<Store>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return;
    }

    this->mStore = info[0].As<Napi::External<GSGridStore>>().Data();
}

Napi::Value Store::putContainer(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    int length = info.Length();
    bool modifiable = false;
    ContainerInfo *containerInfo = NULL;
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    switch (length) {
    case 1:
        if (!info[0].IsObject()) {
            PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
        }
        containerInfo =
                    Napi::ObjectWrap<ContainerInfo>::Unwrap(
                            info[0].As<Napi::Object>());
        break;
    case 2:
        if (!info[0].IsObject() || !info[1].IsBoolean()) {
            PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
        }
        containerInfo =
                    Napi::ObjectWrap<ContainerInfo>::Unwrap(
                            info[0].As<Napi::Object>());

        modifiable = info[1].As<Napi::Boolean>().ToBoolean();
        break;
    default:
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }

    // Get Container information
    GSContainerInfo* gsInfo = containerInfo->gs_info();
    GSContainer* pContainer = NULL;
    // Create new gsContainer
    GSResult ret = gsPutContainerGeneral(
            mStore, gsInfo->name, gsInfo, modifiable, &pContainer);
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }

    // Create new Container object
    Napi::EscapableHandleScope scope(env);
    auto containerPtr = Napi::External<GSContainer>::New(env, pContainer);
    auto containerInfoPtr = Napi::External<GSContainerInfo >::New(env, gsInfo);
    Napi::Value containerWrapper;
    containerWrapper = scope.Escape(Container::constructor.New(
            {containerPtr, containerInfoPtr})).ToObject();
    // Return promise object
    deferred.Resolve(containerWrapper);
    return deferred.Promise();
}

Napi::Value Store::dropContainer(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsString()) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }
    std::string name = info[0].As<Napi::String>().Utf8Value();
    GSResult ret = gsDropContainer(mStore, name.c_str());

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }

    // Return promise object
    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Store::getContainer(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsString()) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }
    std::string name = info[0].As<Napi::String>().Utf8Value();

    GSContainer* pContainer;
    GSResult ret = gsGetContainerGeneral(mStore, name.c_str(), &pContainer);

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }
    if (pContainer == NULL) {
        deferred.Resolve(env.Null());
        return deferred.Promise();
    }
    GSContainerInfo containerInfo = GS_CONTAINER_INFO_INITIALIZER;
    GSChar bExists;
    ret = gsGetContainerInfo(mStore, name.c_str(), &containerInfo, &bExists);
    if (!GS_SUCCEEDED(ret)) {
        gsCloseContainer(&pContainer, GS_FALSE);
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }

    // Create new Container object
    Napi::EscapableHandleScope scope(env);
    auto containerPtr = Napi::External<GSContainer>::New(env, pContainer);
    auto containerInfoPtr =
            Napi::External<GSContainerInfo >::New(env, &containerInfo);
    Napi::Value containerWrapper;
    containerWrapper = scope.Escape(Container::constructor.New({ containerPtr,
            containerInfoPtr })).ToObject();

    // Return promise object
    deferred.Resolve(containerWrapper);
    return deferred.Promise();
}

Store::~Store() {
    if (mStore != NULL) {
        gsCloseGridStore(&mStore, GS_FALSE);
        mStore = NULL;
    }
}

Napi::Value Store::getContainerInfo(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsString()) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }
    std::string name = info[0].As<Napi::String>().Utf8Value();

    GSContainerInfo gsContainerInfo = GS_CONTAINER_INFO_INITIALIZER;
    GSChar bExists;
    GSResult ret = gsGetContainerInfo(
            mStore, name.c_str(), &gsContainerInfo, &bExists);

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }
    if (bExists == false) {
        deferred.Resolve(env.Null());
        return deferred.Promise();
    }
    // Create new ContainerInfo object
    Napi::EscapableHandleScope scope(env);
    auto containerInfoPtr = Napi::External<GSContainerInfo>::New(env,
            &gsContainerInfo);
    Napi::Value containerInfoWrapper = scope.Escape(
            ContainerInfo::constructor.New({containerInfoPtr})).ToObject();
    // Return promise object
    deferred.Resolve(containerInfoWrapper);
    return deferred.Promise();
}

Napi::Value Store::getPartitionController(
        const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    GSPartitionController* partitionController;

    GSResult ret = gsGetPartitionController(mStore, &partitionController);

    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mStore)
        return env.Null();
    }

    // Create new PartitionController object
    Napi::EscapableHandleScope scope(env);
    auto controllerPtr = Napi::External<GSPartitionController>::New(info.Env(),
            partitionController);
    return scope.Escape(PartitionController::constructor.New( {
        controllerPtr })).ToObject();
}

void Store::setReadonlyAttribute(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    Napi::Env env = info.Env();
    THROW_EXCEPTION_WITH_STR(env, "Can't set read only attribute", mStore)
}

}  // namespace griddb

