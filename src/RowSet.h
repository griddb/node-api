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

#ifndef ROWSET_H
#define ROWSET_H

#include <napi.h>
#include "AggregationResult.h"
#include "Macro.h"

namespace griddb {

class RowSet: public Napi::ObjectWrap<RowSet> {
 public:
    // Constructor static variable
    static Napi::FunctionReference constructor;
    static Napi::Object init(Napi::Env env, Napi::Object exports);

    explicit RowSet(const Napi::CallbackInfo &info);
    ~RowSet();

    // NAPI-methods
    Napi::Value hasNext(const Napi::CallbackInfo &info);
    Napi::Value next(const Napi::CallbackInfo &info);
    void setReadonlyAttribute(const Napi::CallbackInfo &info,
            const Napi::Value &value);
    Napi::Value getType(const Napi::CallbackInfo &info);
    Napi::Value getSize(const Napi::CallbackInfo &info);
    GSAggregationResult* getNextAggregation(Napi::Env env);

 private:
    GSRowSet *mRowSet;
    GSContainerInfo *mContainerInfo;
    GSRow *mRow;
    GSType* typeList;
    GSRowSetType mType;
    bool hasNext();
    GSRowSetType type();
    void nextRow(Napi::Env env, bool* hasNextRow);
};

}  // namespace griddb

#endif  // ROWSET_H
