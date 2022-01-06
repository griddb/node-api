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

#ifndef _AGGREGATIONRESULT_H_
#define _AGGREGATIONRESULT_H_

#include <napi.h>
#include "gridstore.h"
#include "Util.h"

namespace griddb {

class AggregationResult : public Napi::ObjectWrap<AggregationResult> {
 public:
#if NAPI_VERSION <= 5
    // Constructor static variable
    static Napi::FunctionReference constructor;
#endif
    static Napi::Object init(Napi::Env env, Napi::Object exports);

    explicit AggregationResult(const Napi::CallbackInfo &info);
    ~AggregationResult();

    // N-API method
    Napi::Value get(const Napi::CallbackInfo &info);

 private:
    GSAggregationResult* mAggResult;
};

}  // namespace griddb

#endif  // _AGGREGATIONRESULT_H_
