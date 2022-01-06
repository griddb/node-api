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

#include "StoreFactory.h"

namespace griddb {

#if NAPI_VERSION <= 5
Napi::FunctionReference StoreFactory::constructor;
#endif

Napi::Object StoreFactory::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "StoreFactory", {
            StaticMethod("getInstance", &StoreFactory::getInstance),
            InstanceMethod("getStore", &StoreFactory::getStore),
            InstanceMethod("getVersion", &StoreFactory::getVersion)
        });
#if NAPI_VERSION > 5
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    Util::setInstanceData(env, "StoreFactory", constructor);
#else
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
#endif
    exports.Set("StoreFactory", func);
    return exports;
}

StoreFactory::StoreFactory(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<StoreFactory>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return;
    }

    this->factory = info[0].As<Napi::External<GSGridStoreFactory>>().Data();
}

/*
 * Check whether in MULTICAST mode
 */
static bool checkMulticast(const char* address) {
    if (address && address[0] != '\0') {
        const char* tmp = reinterpret_cast<const char*>(Util::strdup(address));
        char *octets;
#if defined __USE_POSIX || defined __USE_MISC
        char *savePtr;
        octets = strtok_r(const_cast<char*>(tmp), ".", &savePtr);
#else
        octets = strtok(const_cast<char*>(tmp), ".");
#endif
        if (octets) {
            int firstOctet = atoi(octets);
            int first4Bits = firstOctet >> 4 & 0x0f;
            if (first4Bits == 0x0E) {
                delete[] tmp;
                return true;
            }
        }
        delete[] tmp;
    }
    return false;
}

Napi::Value StoreFactory::getStore(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() != 1 || !info[0].IsObject()) {
        THROW_EXCEPTION_WITH_STR(env, "Wrong argument", factory)
        return env.Null();
    }

    size_t idx = 0;
    GSPropertyEntry properties[8] = {NULL, NULL};

    Napi::Object input = info[0].As<Napi::Object>();
    std::string host;
    std::string port;
    std::string clusterName;
    std::string database;
    std::string username;
    std::string password;
    std::string notificationMember;
    std::string notificationProvider;

    ADD_MEMBER_OPTIONAL_STRING(properties, idx, "host", input, host)
    ADD_MEMBER_OPTIONAL_INT32(properties, idx, "port", input, port)
    ADD_MEMBER_OPTIONAL_STRING(properties, idx, "clusterName", input,
            clusterName)
    ADD_MEMBER_OPTIONAL_STRING(properties, idx, "database", input, database)
    ADD_MEMBER_OPTIONAL_STRING_WITH_KEY(properties, idx, "username", input,
            "user", username)
    ADD_MEMBER_OPTIONAL_STRING(properties, idx, "password", input, password)
    ADD_MEMBER_OPTIONAL_STRING(properties, idx, "notificationMember", input,
            notificationMember)
    ADD_MEMBER_OPTIONAL_STRING(properties, idx, "notificationProvider",
            input, notificationProvider)
    if (checkMulticast(host.c_str())) {
        properties[0].name = "notificationAddress";
        if (!port.empty()) {
            properties[1].name = "notificationPort";
        }
    }

    GSGridStore *store = NULL;

    GSResult ret = gsGetGridStore(factory, properties, idx, &store);
    ENSURE_SUCCESS(gsGetGridStore, ret, factory)

    // Create new Store object
    Napi::EscapableHandleScope scope(env);
    auto storeNode = Napi::External<GSGridStore>::New(env, store);
#if NAPI_VERSION > 5
    return scope.Escape(Util::getInstanceData(env, "Store")->New( {storeNode})).ToObject();
#else
    return scope.Escape(Store::constructor.New( {storeNode})).ToObject();
#endif
}

Napi::Value StoreFactory::getInstance(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    GSGridStoreFactory *factory = gsGetDefaultFactory();

    Napi::EscapableHandleScope scope(env);
    auto arg = Napi::External<GSGridStoreFactory>::New(env, factory);
#if NAPI_VERSION > 5
    return scope.Escape(Util::getInstanceData(env, "StoreFactory")->New( { arg })).ToObject();
#else
    return scope.Escape(StoreFactory::constructor.New( { arg })).ToObject();
#endif
}

StoreFactory::~StoreFactory() {
    if (factory != NULL) {
        gsCloseFactory(&factory, GS_FALSE);
        factory = NULL;
    }
}

Napi::Value StoreFactory::getVersion(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (info.Length() > 0) {
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", factory)
        return env.Null();
    }

    return Napi::String::New(env, CLIENT_VERSION);
}

}  // namespace griddb
