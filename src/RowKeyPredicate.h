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

#ifndef ROWKEYPREDICATE_H
#define ROWKEYPREDICATE_H

#include <napi.h>
#include "Field.h"
#include "Util.h"
#include "gridstore.h"

namespace griddb {

class RowKeyPredicate: public Napi::ObjectWrap<RowKeyPredicate> {
 public:
#if NAPI_VERSION <= 5
    // Constructor static variable
    static Napi::FunctionReference constructor;
#endif
    static Napi::Object init(Napi::Env env, Napi::Object exports);
    explicit RowKeyPredicate(const Napi::CallbackInfo &info);
    ~RowKeyPredicate();

    // N-API methods
    void setRange(const Napi::CallbackInfo &info);
    void setDistinctKeys(const Napi::CallbackInfo &info);
    Napi::Value getRange(const Napi::CallbackInfo &info);
    Napi::Value getDistinctKeys(const Napi::CallbackInfo &info);
    Napi::Value getKeyType(const Napi::CallbackInfo &info);
    GSRowKeyPredicate* getPredicate();

 private:
    GSRowKeyPredicate *predicate;
    GSType type;
};

}  // namespace griddb

#endif  // ROWKEYPREDICATE_H
