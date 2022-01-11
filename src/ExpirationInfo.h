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

#ifndef EXPIRATIONINFO_H
#define EXPIRATIONINFO_H

#include <napi.h>
#include "gridstore.h"

namespace griddb {

class ExpirationInfo: public Napi::ObjectWrap<ExpirationInfo> {
 public:
#if NAPI_VERSION <= 5
    // Constructor
    static Napi::FunctionReference constructor;
#endif
    explicit ExpirationInfo(const Napi::CallbackInfo &info);
    static void init(Napi::Env env, Napi::Object exports);

    // Destructor
    ~ExpirationInfo();

    // N-API method to set/get attribute
    void setTime(const Napi::CallbackInfo &info, const Napi::Value &value);
    Napi::Value getTime(const Napi::CallbackInfo &info);
    void setUnit(const Napi::CallbackInfo &info, const Napi::Value &value);
    Napi::Value getUnit(const Napi::CallbackInfo &info);
    void setDivisionCount(const Napi::CallbackInfo &info,
            const Napi::Value &value);
    Napi::Value getDivisionCount(const Napi::CallbackInfo &info);

    // Support function
    GSTimeSeriesProperties* gs_ts();
 private:
    GSTimeSeriesProperties mTimeSeriesProps;
};

}  // namespace griddb

#endif  // EXPIRATIONINFOWRAPPER_H
