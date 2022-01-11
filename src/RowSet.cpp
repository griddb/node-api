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

#include "RowSet.h"

namespace griddb {

#if NAPI_VERSION <= 5
Napi::FunctionReference RowSet::constructor;
#endif

Napi::Object RowSet::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "RowSet",
            {   InstanceMethod("hasNext", &RowSet::hasNext),
                InstanceMethod("next", &RowSet::next),
                InstanceAccessor("type", &RowSet::getType, &
                        RowSet::setReadonlyAttribute),
                InstanceAccessor("size", &RowSet::getSize,
                        &RowSet::setReadonlyAttribute) });

#if NAPI_VERSION > 5
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    Util::setInstanceData(env, "RowSet", constructor);
#else
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
#endif
    exports.Set("RowSet", func);
    return exports;
}

RowSet::RowSet(const Napi::CallbackInfo& info) :
        Napi::ObjectWrap<RowSet>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 3 || !info[0].IsExternal() || !info[1].IsExternal()
            || !info[2].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return;
    }

    mRowSet = info[0].As<Napi::External<GSRowSet>>().Data();
    mContainerInfo = info[1].As<Napi::External<GSContainerInfo >>().Data();
    mRow = info[2].As<Napi::External<GSRow >>().Data();
    if (mRowSet != NULL) {
        mType = gsGetRowSetType(mRowSet);
    }

    try {
        typeList = new GSType[mContainerInfo->columnCount]();
    } catch (std::bad_alloc&) {
        THROW_EXCEPTION_WITH_STR(env, "Memory allocation error", mRowSet)
        return;
    }

    for (int i = 0; i < static_cast<int>(mContainerInfo->columnCount); i++) {
        typeList[i] = mContainerInfo->columnInfoList[i].type;
    }
}

Napi::Value RowSet::hasNext(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    bool hasNext = this->hasNext();
    return Napi::Boolean::New(env, hasNext);
}

Napi::Value RowSet::next(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    GSRowSetType type = mType;
    bool hasNextRow;
    GSAggregationResult *aggResult = NULL;
    GSQueryAnalysisEntry *queryResult = NULL;
    GSQueryAnalysisEntry gsQueryAnalysis = GS_QUERY_ANALYSIS_ENTRY_INITIALIZER;
    switch (type) {
    case (GS_ROW_SET_CONTAINER_ROWS):
        this->nextRow(env, &hasNextRow);
        break;
    case (GS_ROW_SET_AGGREGATION_RESULT):
        hasNextRow = this->hasNext();
        aggResult = this->getNextAggregation(env);
        break;
    case (GS_ROW_SET_QUERY_ANALYSIS): {
        queryResult = &gsQueryAnalysis;
        this->getNextQueryAnalysis(env, &queryResult);
        hasNextRow = true;
        break;
    }
    default:
        THROW_EXCEPTION_WITH_STR(env, "type for rowset is not correct",
                mRowSet)
        return env.Null();
    }

    if (!hasNextRow) {
        return env.Null();
    }

    Napi::Value returnWrapper;
    switch (type) {
    case GS_ROW_SET_CONTAINER_ROWS: {
        returnWrapper = Util::fromRow(env, mRow, mContainerInfo->columnCount,
                typeList);
        break;
    }
    case GS_ROW_SET_AGGREGATION_RESULT: {
        Napi::EscapableHandleScope scope(env);
        auto aggPtr = Napi::External<GSAggregationResult>::New(env,
                aggResult);
#if NAPI_VERSION > 5
        returnWrapper = scope.Escape(
                Util::getInstanceData(env, "AggregationResult")->New({aggPtr})).ToObject();
#else
        returnWrapper = scope.Escape(
                AggregationResult::constructor.New({aggPtr})).ToObject();
#endif
        break;
    }
    case GS_ROW_SET_QUERY_ANALYSIS: {
        Napi::EscapableHandleScope scope(env);
        auto queryPtr = Napi::External<GSQueryAnalysisEntry>::New(env,
                queryResult);
#if NAPI_VERSION > 5
        returnWrapper = scope.Escape(
                Util::getInstanceData(env, "QueryAnalysisEntry")->New({queryPtr})).ToObject();
#else
        returnWrapper = scope.Escape(
                QueryAnalysisEntry::constructor.New({queryPtr})).ToObject();
#endif
        break;
    }

    default:
        THROW_EXCEPTION_WITH_STR(env, "Type is not support", mRowSet)
        return env.Null();
    }

    return returnWrapper;
}

RowSet::~RowSet() {
    if (mRowSet != NULL) {
        gsCloseRowSet(&mRowSet);
        mRowSet = NULL;
    }
    if (typeList) {
        delete[] typeList;
    }
}

/**
 *  Throw exception when set value to readonly attribute
 */
void RowSet::setReadonlyAttribute(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    Napi::Env env = info.Env();
    THROW_EXCEPTION_WITH_STR(env, "Can't set read only attribute", mRowSet)
}

Napi::Value RowSet::getType(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    return Napi::Number::New(env, mType);
}

Napi::Value RowSet::getSize(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    int32_t size = gsGetRowSetSize(mRowSet);
    return Napi::Number::New(env, size);
}

/**
* @brief Check if RowSet has next row data.
* @return Returns whether a Row set has at least one Row ahead of the current cursor position
*/
bool RowSet::hasNext() {
    GSRowSetType type;
    type = this->type();
    switch (type) {
    case (GS_ROW_SET_CONTAINER_ROWS):
    case (GS_ROW_SET_AGGREGATION_RESULT):
    case (GS_ROW_SET_QUERY_ANALYSIS):
        return static_cast<bool>(gsHasNextRow(mRowSet));
    default:
        return false;
    }
}

/**
 * @brief Get current row type.
 * @return The type of content that can be extracted from GSRowSet.
 */
GSRowSetType RowSet::type() {
    return mType;
}

/**
 * @brief Get next row data.
 * @param *hasNextRow Indicate whether there is any row in RowSet or not
 */
void RowSet::nextRow(Napi::Env env, bool* hasNextRow) {
    *hasNextRow = this->hasNext();
    if (*hasNextRow) {
        GSResult ret = gsGetNextRow(mRowSet, mRow);
        if (!GS_SUCCEEDED(ret)) {
            THROW_EXCEPTION_WITH_CODE(env, ret, mRowSet)
        }
    }
}

/**
 * @brief Moves to the next Row in a Row set and returns the aggregation result at the moved position.
 * @return A pointer Stores the result of an aggregation operation.
 */
GSAggregationResult* RowSet::getNextAggregation(Napi::Env env) {
    GSAggregationResult* pAggResult;

    GSResult ret = gsGetNextAggregation(mRowSet, &pAggResult);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mRowSet)
        return NULL;
    }

    return pAggResult;
}

void RowSet::getNextQueryAnalysis(Napi::Env env,
        GSQueryAnalysisEntry **queryResult) {
    GSResult ret = gsGetNextQueryAnalysis(mRowSet, *queryResult);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mRowSet)
        return;
    }
}

}  // namespace griddb

