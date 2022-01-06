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
#include "GSException.h"
#include "Util.h"

namespace griddb {
#if NAPI_VERSION <= 5
Napi::FunctionReference GSException::constructor;
#endif
Napi::Object GSException::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);
    Napi::Function t = DefineClass(env, "GSException", {
        InstanceMethod("isTimeout", &GSException::isTimeout),
        InstanceMethod("getStackError", &GSException::getStackError),
        InstanceMethod("getErrorStackSize", &GSException::getErrorStackSize),
        InstanceMethod("getErrorCode", &GSException::getErrorCode),
        InstanceMethod("getMessage", &GSException::getMessage),
        InstanceMethod("getLocation", &GSException::getLocation)
    });
#if NAPI_VERSION > 5
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(t);
    Util::setInstanceData(env, "GSException", constructor);
#else
    constructor = Napi::Persistent(t);
    constructor.SuppressDestruct();
#endif
    exports.Set("GSException", t);
    return exports;
}
GSException::GSException(const Napi::CallbackInfo& info) :
        Napi::ObjectWrap<GSException>(info),
        mCode(DEFAULT_ERROR_CODE),
        mMessage(""),
        mLocation(""),
        mResource(nullptr),
        mStackSize(DEFAULT_ERROR_STACK_SIZE) {
    Napi::Env env = info.Env();
    if (info.Length() != 4) {
        throw Napi::Error::New(env, "Wrong error type");
        return;
    }
    REQUIRE_ARGUMENT_NUMBER(0, mCode);
    REQUIRE_ARGUMENT_STRING(1, mMessage);
    REQUIRE_ARGUMENT_STRING(2, mLocation);
    REQUIRE_ARGUMENT_EXTERNAL(3, mResource, void);

    if (mResource != NULL) {
        mStackSize = gsGetErrorStackSize(mResource);
    }
    Napi::Object This = info.This().As<Napi::Object>();
    This.DefineProperties({
        DEFINE_CONSTANT_INTEGER(This, mCode, mCode)
        DEFINE_CONSTANT_STRING(This, mMessage, mMessage)
        DEFINE_CONSTANT_STRING(This, mLocation, mLocation)
        DEFINE_CONSTANT_INTEGER(This, mStackSize, mStackSize)
    });
}
Napi::Object GSException::New(Napi::Env env, GSResult code, void* resource) {
    std::string message = std::string(
            "Error with number ") + std::to_string(code);
    return New(env, code, message.c_str(), NULL, resource);
}

Napi::Object GSException::New(Napi::Env env, std::string message,
        void* resource) {
    return New(env, DEFAULT_ERROR_CODE, message.c_str(), NULL, resource);
}
Napi::Object GSException::New(Napi::Env env, GSResult code,
        const char* message, const char* location, void* resource) {
#if NAPI_VERSION > 5
    return Util::getInstanceData(env, "GSException")->New({
        Napi::Number::New(env, code),
        message  ? Napi::String::New(env, message) : env.Null(),
        location ? Napi::String::New(env, location) : env.Null(),
        resource ? Napi::External<void>::New(env, resource) : env.Null()
    });
#else
    return constructor.New( {
        Napi::Number::New(env, code),
        message  ? Napi::String::New(env, message) : env.Null(),
        location ? Napi::String::New(env, location) : env.Null(),
        resource ? Napi::External<void>::New(env, resource) : env.Null()
    });
#endif
}
GSException::~GSException() {
}
Napi::Value GSException::isTimeout(const Napi::CallbackInfo& info) {
    GSBool value = gsIsTimeoutError(mCode);
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, value);
}

Napi::Value GSException::getStackError(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsNumber()) {
        throw Napi::TypeError::New(env, "Wrong argument type");
        return env.Null();
    }
    if (mResource == NULL || mStackSize == 0) {
        return env.Null();
    }
    int64_t index = info[0].As<Napi::Number>().Int64Value();
    if (index < 0 || index > static_cast<int>(mStackSize)) {
        throw Napi::TypeError::New(env, "Wrong argument value");
        return env.Null();
    }

    GSChar buffer[BUFF_SIZE] = {0};
    GSResult stackCode = gsGetErrorCode(mResource, index);
    std::string stackMessage;
    std::string stackLocation;

    size_t length = gsFormatErrorMessage(mResource, index, buffer, BUFF_SIZE);
    if (length > 0) {
        stackMessage = std::string(buffer, length);
    }
    length = gsFormatErrorLocation(mResource, index, buffer, BUFF_SIZE);
    if (length > 0) {
        stackLocation = std::string(buffer, length);
    }
    return GSException::New(env, stackCode, stackMessage.c_str(),
            stackLocation.c_str(), NULL);
}

Napi::Value GSException::getErrorStackSize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() != 0) {
        throw Napi::TypeError::New(env, "Wrong argument type");
        return env.Null();
    }

    return Napi::Number::New(env, mStackSize);
}

Napi::Value GSException::getErrorCode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Wrong argument type").
                ThrowAsJavaScriptException();
        return env.Null();
    }
    size_t stackIndex = info[0].As<Napi::Number>().Int32Value();
    GSResult errorCode;

    if (stackIndex == 0)  {
        errorCode = mCode;
    } else if (stackIndex >= mStackSize) {
        errorCode = 0;
    } else {
        errorCode = gsGetErrorCode(mResource, stackIndex);
    }
    return Napi::Number::New(env, errorCode);
}

Napi::Value GSException::getMessage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Wrong argument type").
                ThrowAsJavaScriptException();
        return env.Null();
    }
    size_t stackIndex = info[0].As<Napi::Number>().Int32Value();
    std::string ret;

    if (stackIndex == 0) {
        ret = mMessage;
    } else if (stackIndex >= mStackSize) {
        ret = "";
    } else {
        char* strBuf = new char[BUFF_SIZE];
        size_t stringSize = gsFormatErrorMessage(
                mResource, stackIndex, strBuf, BUFF_SIZE);
        ret = std::string(strBuf, stringSize);
        delete [] strBuf;
    }

    return Napi::String::New(env, ret);
}

Napi::Value GSException::getLocation(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Wrong argument type").
                ThrowAsJavaScriptException();
        return env.Null();
    }
    size_t stackIndex = info[0].As<Napi::Number>().Int32Value();
    std::string ret;

    if (stackIndex >= mStackSize) {
        ret = "";
    } else {
        char* strBuf = new char[BUFF_SIZE];
        size_t stringSize = gsFormatErrorLocation(
                mResource, stackIndex, strBuf, BUFF_SIZE);
        ret = std::string(strBuf, stringSize);
        delete [] strBuf;
    }
    return Napi::String::New(env, ret);
}

}  // namespace griddb
