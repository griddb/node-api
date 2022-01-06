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

#include "Query.h"

namespace griddb {

#if NAPI_VERSION <= 5
Napi::FunctionReference Query::constructor;
#endif

Napi::Object Query::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Query",
            { InstanceMethod("fetch", &Query::fetch),
              InstanceMethod("setFetchOptions", &Query::setFetchOptions),
              InstanceMethod("getRowSet", &Query::getRowSet),
            });

#if NAPI_VERSION > 5
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    Util::setInstanceData(env, "Query", constructor);
#else
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
#endif
    exports.Set("Query", func);
    return exports;
}

Query::Query(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<Query>(info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    if (info.Length() != 3 || !info[0].IsExternal() || !info[1].IsExternal()
            || !info[2].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return;
    }
    this->mQuery = info[0].As<Napi::External<GSQuery>>().Data();
    this->mContainerInfo = info[1].As<Napi::External<GSContainerInfo>>().Data();
    this->mRow = info[2].As<Napi::External<GSRow>>().Data();
}

Napi::Value Query::fetch(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    GSRowSet *gsRowSet;
    // Call method from C-Api.
    GSBool gsForUpdate = GS_FALSE;
    GSResult ret = gsFetch(mQuery, gsForUpdate, &gsRowSet);

    // Check ret, if error, throw exception
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mQuery)
    }

    // Create new RowSet object
    Napi::EscapableHandleScope scope(env);
    auto rowsetPtr = Napi::External<GSRowSet>::New(env, gsRowSet);
    auto gsContainerInfoPtr =
            Napi::External<GSContainerInfo>::New(env, mContainerInfo);
    auto gsRowPtr = Napi::External<GSRow>::New(env, mRow);
#if NAPI_VERSION > 5
    Napi::Value rowsetWrapper = scope.Escape(
            Util::getInstanceData(env, "RowSet")->New({
                    rowsetPtr, gsContainerInfoPtr, gsRowPtr })).ToObject();
#else
    Napi::Value rowsetWrapper = scope.Escape(RowSet::constructor.New({
            rowsetPtr, gsContainerInfoPtr, gsRowPtr })).ToObject();
#endif
    deferred.Resolve(rowsetWrapper);
    return deferred.Promise();
}

Query::~Query() {
    if (mQuery) {
        gsCloseQuery(&mQuery);
        mQuery = NULL;
    }
}

void Query::setFetchOptions(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (info.Length() > 1 || !info[0].IsObject()) {
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", mQuery)
        return;
    }
    Napi::Object input = info[0].As<Napi::Object>();

    int limit = 0;
    if (input.Has("limit")) {
        limit = input.Get("limit").As<Napi::Number>().Int32Value();
    }

    GSResult ret;
    ret = gsSetFetchOption(mQuery, GS_FETCH_LIMIT, &limit, GS_TYPE_INTEGER);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mQuery)
        return;
    }
#if GS_COMPATIBILITY_SUPPORT_4_0
    bool partial = true;  // default value rowKey = true
    if (input.Has("partial")) {
        partial = input.Get("partial").As<Napi::Boolean>().ToBoolean();
    }
    // Need to call gsSetFetchOption as many as the number of options
    ret = gsSetFetchOption(mQuery, GS_FETCH_PARTIAL_EXECUTION, &partial,
            GS_TYPE_BOOL);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mQuery)
        return;
    }
#endif
}

Napi::Value Query::getRowSet(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    GSRowSet *gsRowSet;
    GSResult ret = gsGetRowSet(mQuery, &gsRowSet);

    // Check ret, if error, throw exception
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mQuery)
        return env.Null();
    }
    auto rowset_ptr = Napi::External<GSRowSet>::New(env, gsRowSet);
    auto containerInfo_ptr = Napi::External<GSContainerInfo>
                ::New(env, mContainerInfo);
    auto row_ptr = Napi::External<GSRow>::New(env, mRow);
    Napi::EscapableHandleScope scope(env);
#if NAPI_VERSION > 5
    return scope.Escape(Util::getInstanceData(env, "RowSet")->New({
            rowset_ptr, containerInfo_ptr, row_ptr })).ToObject();
#else
    return scope.Escape(RowSet::constructor.New({
            rowset_ptr, containerInfo_ptr, row_ptr })).ToObject();
#endif
}

GSQuery* Query::gsPtr() {
    return mQuery;
}

}  // namespace griddb

