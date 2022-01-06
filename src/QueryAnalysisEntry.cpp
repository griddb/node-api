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
#include "QueryAnalysisEntry.h"

#include <assert.h>

namespace griddb {

#if NAPI_VERSION <= 5
Napi::FunctionReference QueryAnalysisEntry::constructor;
#endif

Napi::Object QueryAnalysisEntry::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "QueryAnalysisEntry",
            { InstanceMethod("get", &QueryAnalysisEntry::get),
            });

#if NAPI_VERSION > 5
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    Util::setInstanceData(env, "QueryAnalysisEntry", constructor);
#else
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
#endif
    exports.Set("QueryAnalysisEntry", func);
    return exports;
}

QueryAnalysisEntry::QueryAnalysisEntry(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<QueryAnalysisEntry>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return;
    }

    GSQueryAnalysisEntry* queryAnalysPtr = info[0].As<Napi::External
                <GSQueryAnalysisEntry>>().Data();
    mQueryAnalysis.depth = queryAnalysPtr->depth;
    mQueryAnalysis.id = queryAnalysPtr->id;
    mQueryAnalysis.statement = Util::strdup(queryAnalysPtr->statement);
    mQueryAnalysis.type = Util::strdup(queryAnalysPtr->type);
    mQueryAnalysis.value = Util::strdup(queryAnalysPtr->value);
    mQueryAnalysis.valueType = Util::strdup(queryAnalysPtr->valueType);
}

Napi::Value QueryAnalysisEntry::get(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (info.Length() != 0) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return env.Null();
    }

    Napi::Array arrResult = Napi::Array::New(env);
    arrResult.Set("0", mQueryAnalysis.id);
    arrResult.Set("1", mQueryAnalysis.depth);
    arrResult.Set("2", mQueryAnalysis.type);
    arrResult.Set("3", mQueryAnalysis.valueType);
    arrResult.Set("4", mQueryAnalysis.value);
    arrResult.Set("5", mQueryAnalysis.statement);
    return arrResult;
}

QueryAnalysisEntry::~QueryAnalysisEntry() {
    // Free memory from Util::strdup
    if (mQueryAnalysis.statement) {
        delete mQueryAnalysis.statement;
    }
    if (mQueryAnalysis.type) {
        delete mQueryAnalysis.type;
    }
    if (mQueryAnalysis.value) {
        delete mQueryAnalysis.value;
    }
    if (mQueryAnalysis.valueType) {
        delete mQueryAnalysis.valueType;
    }
}

}  // namespace griddb

