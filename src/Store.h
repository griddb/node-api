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

#ifndef STORE_H
#define STORE_H

#include <napi.h>
#include <limits>
#include "Container.h"
#include "ContainerInfo.h"
#include "PartitionController.h"
#include "RowKeyPredicate.h"

#include "Util.h"

namespace griddb {

class Store: public Napi::ObjectWrap<Store> {
 public:
    // Constructor static variable
    static Napi::FunctionReference constructor;
    static Napi::Object init(Napi::Env env, Napi::Object exports);

    explicit Store(const Napi::CallbackInfo &info);
    ~Store();

    // N-API methods
    Napi::Value putContainer(const Napi::CallbackInfo &info);
    Napi::Value dropContainer(const Napi::CallbackInfo &info);
    Napi::Value getContainer(const Napi::CallbackInfo &info);
    Napi::Value getContainerInfo(const Napi::CallbackInfo &info);
    Napi::Value getPartitionController(const Napi::CallbackInfo &info);
    Napi::Value multiPut(const Napi::CallbackInfo &info);
    Napi::Value multiGet(const Napi::CallbackInfo &info);
    Napi::Value createRowKeyPredicate(const Napi::CallbackInfo &info);
    Napi::Value fetchAll(const Napi::CallbackInfo &info);

    // N-API support methods
    void setReadonlyAttribute(const Napi::CallbackInfo &info,
            const Napi::Value &value);

 private:
    GSGridStore *mStore;
};

}  // namespace griddb

#endif  // STORE_H
