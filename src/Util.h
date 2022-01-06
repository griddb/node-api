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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <map>
#include <string.h>
#include <napi.h>
#include "Field.h"
#include "GSException.h"
#include "gridstore.h"

#define UTC_TIMESTAMP_MAX 253402300799.999  // Max timestamp in seconds
#define MAX_LONG 9007199254740992
#define MIN_LONG -9007199254740992

// Support store data for each instance library when run multi threads
typedef std::map<std::string, Napi::FunctionReference*> AddonData;

class Util {
 public:
    static const GSChar* strdup(const GSChar *from);

    // Convert data from Napi::Value to GSRow field
    static void toField(const Napi::Env &env, Napi::Value *value, GSRow *row,
            int column, GSType type);

    static GSTimestamp toGsTimestamp(const Napi::Env &env, Napi::Value *value);

    // Convert data from GSRow* to Napi::Value
    static Napi::Value fromRow(const Napi::Env &env, GSRow *row,
            int columnCount, GSType *typeList);
    static Napi::Value fromField(const Napi::Env& env, GSRow* row, int column,
            GSType type);
    static Napi::Value fromTimestamp(const Napi::Env& env,
            GSTimestamp *timestamp);

    // Other support methods
    static void freeStrData(Napi::Env env, void* data);
    static void setInstanceData(Napi::Env env, std::string key,
            Napi::FunctionReference *function);
    static Napi::FunctionReference *getInstanceData(Napi::Env env, std::string key);
    static void addonDataFinalizer(Napi::Env env, AddonData* data);
};
#endif  // _UTIL_H_
