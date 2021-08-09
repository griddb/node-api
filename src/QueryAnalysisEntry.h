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

#ifndef QUERYANALYSISENTRY_H
#define QUERYANALYSISENTRY_H

#include <napi.h>
#include <stdio.h>
#include <string.h>

#include "gridstore.h"
#include "Util.h"

namespace griddb {

class QueryAnalysisEntry: public Napi::ObjectWrap<QueryAnalysisEntry> {
 public:
    // Constructor static variable
    static Napi::FunctionReference constructor;
    static Napi::Object init(Napi::Env env, Napi::Object exports);

    explicit QueryAnalysisEntry(const Napi::CallbackInfo &info);
    ~QueryAnalysisEntry();

    // N-API methods
    Napi::Value get(const Napi::CallbackInfo &info);

 private:
    GSQueryAnalysisEntry mQueryAnalysis = GS_QUERY_ANALYSIS_ENTRY_INITIALIZER;
};

}  // namespace griddb

#endif  // QUERYANALYSISENTRY_H

