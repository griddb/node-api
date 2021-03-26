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

#ifndef QUERY_H
#define QUERY_H

#include <napi.h>
#include "Util.h"
#include "RowSet.h"
#include "Macro.h"

namespace griddb {

class Query: public Napi::ObjectWrap<Query> {
 public:
    // Constructor static variable
    static Napi::FunctionReference constructor;
    static Napi::Object init(Napi::Env env, Napi::Object exports);

    explicit Query(const Napi::CallbackInfo &info);
    ~Query();

    // N-API methods
    Napi::Value fetch(const Napi::CallbackInfo &info);
    void setFetchOptions(const Napi::CallbackInfo &info);
    Napi::Value getRowSet(const Napi::CallbackInfo &info);

 private:
    GSQuery *mQuery;
    GSContainerInfo *mContainerInfo;
    GSRow* mRow;
};

}  // namespace griddb

#endif  // QUERY_H
