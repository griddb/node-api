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

#ifndef _MACRO__H_
#define _MACRO__H_

#include <napi.h>
#include <string>

#include "gridstore.h"

template <typename T> void GSDefaultFini(Napi::Env, T* data) {
    delete data;
}

#define THROW_GSEXCEPTION(obj) \
    Napi::Error(obj.Env(), obj).ThrowAsJavaScriptException();

#define OPTIONAL_MEMBER_STRING(var, name, obj)                  \
    if (obj.Has(name) && obj.Get(name).IsString()) {          \
        var = obj.Get(name).As<Napi::String>().Utf8Value();    \
    }

#define OPTIONAL_MEMBER_INT32(var, name, obj)                  \
    if (obj.Has(name) && obj.Get(name).IsNumber()) {          \
        var = obj.Get(name).As<Napi::Number>().Int32Value();    \
    }

#define ADD_MEMBER_OPTIONAL_STRING(props, idx, name, obj, str)        \
    if (obj.Has(name) && obj.Get(name).IsString()) {          \
        str = obj.Get(name).As<Napi::String>().Utf8Value();    \
        props[idx] = {name, str.c_str()};    \
        idx++;  \
    }

#define ADD_MEMBER_OPTIONAL_STRING_WITH_KEY(  \
         props, idx, name, obj, propName, str)        \
    if (obj.Has(name) && obj.Get(name).IsString()) {          \
        str = obj.Get(name).As<Napi::String>().Utf8Value();    \
        props[idx] = {propName, str.c_str()};    \
        idx++;  \
    }

#define ADD_MEMBER_OPTIONAL_INT32(props, idx, name, obj, port)        \
    if (obj.Has(name) && obj.Get(name).IsNumber()) {          \
        int num = obj.Get(name).As<Napi::Number>().Int32Value();    \
        port = std::to_string(num);    \
        props[idx] = {name, port.c_str()};    \
        idx++;  \
    }

#define ENSURE_SUCCESS(method, ret, resource)     \
    if (!GS_SUCCEEDED(ret)) {   \
        std::string msg = "Method " #method " return error "+  \
                std::to_string(ret);    \
        Napi::Object obj = griddb::GSException::New(env, msg, resource);    \
        Napi::Error(obj.Env(), obj).ThrowAsJavaScriptException();    \
    }

#define ENSURE_SUCCESS_CPP(method, ret)     \
    if (!GS_SUCCEEDED(ret)) {   \
        std::string msg = "Method " #method " return error "+   \
                std::to_string(ret);    \
        Napi::Object obj = griddb::GSException::New(env, msg);    \
        throw Napi::Error(env, obj);    \
    }

#define PROMISE_REJECT_WITH_STRING(deferred, env, msg, resource)      \
    Napi::Object obj = griddb::GSException::New(env, msg, resource);    \
    deferred.Reject(Napi::Error(env, obj).Value());    \
    return deferred.Promise();

#define PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, code, resource)      \
    std::string msg = "Error with number " +  \
            std::to_string(code);      \
    Napi::Object obj = griddb::GSException::New(env, msg, resource);    \
    deferred.Reject(Napi::Error(env, obj).Value());    \
    return deferred.Promise();

#define PROMISE_REJECT_WITH_GSEXCEPTION(deferred, env, gsexeption)      \
        deferred.Reject(Napi::Error(env, gsexeption).Value());    \
    return deferred.Promise();

#define PROMISE_REJECT_WITH_ERROR(deferred, obj)      \
        deferred.Reject(obj.Value());    \
    return deferred.Promise();

#define THROW_EXCEPTION_WITH_STR(env, msg, resource)      \
    Napi::Object obj = griddb::GSException::New(env, msg, resource);    \
    Napi::Error(env, obj).ThrowAsJavaScriptException();

#define THROW_CPP_EXCEPTION_WITH_STR(env, msg)      \
    Napi::Object obj = griddb::GSException::New(env, msg);    \
    throw Napi::Error(env, obj);

#define THROW_EXCEPTION_WITH_CODE(env, code, resource)      \
    std::string msg = "Error with number " +  \
            std::to_string(code);  \
    Napi::Object obj = griddb::GSException::New(env, msg, resource);      \
    Napi::Error(obj.Env(), obj).ThrowAsJavaScriptException();

#define REQUIRE_MEMBER_STRING(var, name, obj, env, deferred)      \
    if (!obj.Has(name) || !obj.Get(name).IsString()) {      \
        Napi::Object gsException = \
                griddb::GSException::New(env, "Missing " #name " attribute"); \
        deferred.Reject(      \
            Napi::Error(env, gsException).Value());       \
        return deferred.Promise();      \
    }      \
    var = obj.Get(name).As<Napi::String>().Utf8Value();

#define REQUIRE_ARGUMENT_STRING(i, var) \
    if (info.Length() <= i || (!info[i].IsString() && !info[i].IsNull())) { \
        Napi::Error::New(env, "Argument " #i " is invalid").  \
                ThrowAsJavaScriptException(); \
        return;  \
    } \
    if (!info[i].IsNull()) { \
        var = info[i].As<Napi::String>().Utf8Value(); \
    }
#define REQUIRE_ARGUMENT_NUMBER(i, var) \
    if (info.Length() <= i || (!info[i].IsNumber() && !info[i].IsNull())) { \
        Napi::Error::New(env, "Argument " #i " is invalid").  \
                ThrowAsJavaScriptException(); \
        return;  \
    } \
    if (!info[i].IsNull()) { \
        var = info[i].As<Napi::Number>(); \
    }

#define REQUIRE_ARGUMENT_EXTERNAL(i, var, type) \
    if (info.Length() <= i || (!info[i].IsExternal() && !info[i].IsNull())) { \
        Napi::Error::New  \
            (env, "Argument " #i " is invalid").ThrowAsJavaScriptException(); \
        return;  \
    } \
    if (!info[i].IsNull()) { \
        var = info[i].As<Napi::External<type>>().Data(); \
    }

#define DEFINE_CONSTANT_VALUE(target, constant, name)                        \
    Napi::PropertyDescriptor::Value(#name, constant,   \
        static_cast<napi_property_attributes>          \
                (napi_enumerable | napi_configurable)),
#define DEFINE_CONSTANT_INTEGER(target, constant, name)                        \
    Napi::PropertyDescriptor::Value(#name, Napi::Number::New(env, constant),   \
        static_cast<napi_property_attributes>                                  \
                (napi_enumerable | napi_configurable)),
#define DEFINE_CONSTANT_BOOL(target, constant, name)                        \
    Napi::PropertyDescriptor::Value(#name, Napi::Boolean::New(env, constant),  \
        static_cast<napi_property_attributes>                                  \
                (napi_enumerable | napi_configurable)),
#define DEFINE_CONSTANT_STRING(target, constant, name)                         \
    Napi::PropertyDescriptor::Value(#name, Napi::String::New(env, constant),   \
        static_cast<napi_property_attributes>                                  \
                (napi_enumerable | napi_configurable)),
#define DEFINE_CONSTANT_STRING_0(target, constant, name)                       \
    if (constant != 0) {                                                       \
        target.DefineProperty(Napi::PropertyDescriptor::Value(                 \
                #name, Napi::String::New(env, constant),                       \
                static_cast<napi_property_attributes>                          \
                        (napi_enumerable | napi_configurable))); \
    }

#endif  // _MACRO__H_
