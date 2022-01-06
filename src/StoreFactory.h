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

#ifndef STOREFACTORY_H
#define STOREFACTORY_H
#define CLIENT_VERSION "GridDB Node API 0.8"

#include <napi.h>
#include <string>
#include "gridstore.h"
#include "Macro.h"
#include "Store.h"
#include "Util.h"

namespace griddb {

class StoreFactory: public Napi::ObjectWrap<StoreFactory> {
 public:
#if NAPI_VERSION <= 5
    static Napi::FunctionReference constructor;
#endif
    static Napi::Object init(Napi::Env env, Napi::Object exports);

    explicit StoreFactory(const Napi::CallbackInfo &info);
    ~StoreFactory();

    // N-API methods
    static Napi::Value getInstance(const Napi::CallbackInfo &info);
    Napi::Value getStore(const Napi::CallbackInfo &info);
    Napi::Value getVersion(const Napi::CallbackInfo &info);


 private:
    GSGridStoreFactory *factory;
};

}  // namespace griddb

#endif  // STOREFACTORY_H
