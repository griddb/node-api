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

#ifndef _GS_EXCEPTION_H
#define _GS_EXCEPTION_H
#include <napi.h>
#include <string>
#include "gridstore.h"
#include "Macro.h"

#define DEFAULT_ERROR_CODE -1
#define DEFAULT_ERROR_STACK_SIZE 1
#define BUFF_SIZE 1024

namespace griddb {
class GSException : public Napi::ObjectWrap<GSException>{
 public:
    static Napi::FunctionReference constructor;
    static Napi::Object init(Napi::Env env, Napi::Object exports);
    static Napi::Object New(Napi::Env env, GSResult code,
            void* resource = NULL);
    static Napi::Object New(Napi::Env env, std::string message,
            void* resource = NULL);
    static Napi::Object New(Napi::Env env, GSResult code, const char* message,
            const char* location, void* resource = NULL);
    explicit GSException(const Napi::CallbackInfo& info);
    virtual ~GSException();
    Napi::Value isTimeout(const Napi::CallbackInfo& info);
    Napi::Value getStackError(const Napi::CallbackInfo& info);
    Napi::Value getErrorStackSize(const Napi::CallbackInfo& info);
    Napi::Value getErrorCode(const Napi::CallbackInfo& info);
    Napi::Value getMessage(const Napi::CallbackInfo& info);
    Napi::Value getLocation(const Napi::CallbackInfo& info);

 private:
    GSResult mCode;
    std::string mMessage;
    std::string mLocation;
    void* mResource;
    size_t mStackSize;
};
}  // namespace griddb
#endif  // _GS_EXCEPTION_H
