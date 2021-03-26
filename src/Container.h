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

#ifndef CONTAINER_H
#define CONTAINER_H

#include <napi.h>
#include <node_buffer.h>
#include <limits>
#include "Store.h"
#include "Container.h"
#include "Query.h"
#include "Util.h"
#include "Macro.h"

namespace griddb {

class Container: public Napi::ObjectWrap<Container> {
 public:
    // Contructor static variable
    static Napi::FunctionReference constructor;
    static Napi::Object init(Napi::Env env, Napi::Object exports);

    explicit Container(const Napi::CallbackInfo &info);
    ~Container();

    // N-API methods
    Napi::Value put(const Napi::CallbackInfo &info);
    Napi::Value query(const Napi::CallbackInfo &info);
    Napi::Value get(const Napi::CallbackInfo &info);
    Napi::Value multiPut(const Napi::CallbackInfo &info);
    Napi::Value createIndex(const Napi::CallbackInfo &info);
    Napi::Value dropIndex(const Napi::CallbackInfo &info);
    Napi::Value flush(const Napi::CallbackInfo &info);
    Napi::Value abort(const Napi::CallbackInfo &info);
    Napi::Value commit(const Napi::CallbackInfo &info);
    Napi::Value setAutoCommit(const Napi::CallbackInfo &info);
    Napi::Value remove(const Napi::CallbackInfo &info);
    Napi::Value getType(const Napi::CallbackInfo &info);

 private:
    GSContainerInfo* mContainerInfo;
    GSContainer *mContainer;
    GSRow* mRow;
    GSType* mTypeList;
};

}  // namespace griddb

#endif  // CONTAINER_H
