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

#ifndef CONTAINERINFO_H
#define CONTAINERINFO_H

#include <napi.h>
#include "ExpirationInfo.h"
#include "gridstore.h"
#include "Util.h"
#include "GSException.h"
#include "Macro.h"

// Support column_info_list attribute
struct ColumnInfoList {
    GSColumnInfo* columnInfo;
    size_t size;
};

namespace griddb {

class ContainerInfo: public Napi::ObjectWrap<ContainerInfo> {
 public:
#if NAPI_VERSION <= 5
    // Constructor
    static Napi::FunctionReference constructor;
#endif
    explicit ContainerInfo(const Napi::CallbackInfo &info);
    static Napi::Object init(Napi::Env env, Napi::Object exports);

    // Destructor
    ~ContainerInfo();

    // NAPI methods to set/get attribute
    void setName(const Napi::CallbackInfo &info, const Napi::Value &value);
    Napi::Value getName(const Napi::CallbackInfo &info);
    void setType(const Napi::CallbackInfo &info, const Napi::Value &value);
    Napi::Value getType(const Napi::CallbackInfo &info);
    void setRowKeyAssigned(const Napi::CallbackInfo &info,
            const Napi::Value &value);
    Napi::Value getRowKeyAssigned(const Napi::CallbackInfo &info);
    void setColumnInfoList(const Napi::CallbackInfo &info,
            const Napi::Value &value);
    Napi::Value getColumnInfoList(const Napi::CallbackInfo &info);
    void setExpiration(const Napi::CallbackInfo &info,
            const Napi::Value &value);
    Napi::Value getExpiration(const Napi::CallbackInfo &info);

    // NAPI support methods
    ContainerInfo* getContainerInfo();
    GSContainerInfo* gs_info();

 private:
    GSContainerInfo mContainerInfo;

    // Tmp attribute to get column info list
    ColumnInfoList mColumnInfoList;

    // Tmp attribute support get expiration attribute
    ExpirationInfo* mExpInfo;
    void freeMemoryPropsList(GSColumnInfo *props, int propsCount);
    void freeColumnInfoList(ColumnInfoList* columnInfoList);
    void init(const Napi::Env &env, const GSChar* name, GSContainerType type,
            const GSColumnInfo* props, int propsCount, bool rowKeyAssigned,
            ExpirationInfo* expiration);
};

}  // namespace griddb

#endif  // CONTAINERINFO_H
