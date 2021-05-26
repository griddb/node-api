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

#include "Container.h"
#include <string>

namespace griddb {

Napi::FunctionReference Container::constructor;

Napi::Object Container::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Container",
            {   InstanceMethod("put", &Container::put),
                InstanceMethod("query", &Container::query),
                InstanceMethod("get", &Container::get),
                InstanceMethod("queryByTimeSeriesRange",
                    &Container::queryByTimeSeriesRange),
                InstanceMethod("multiPut", &Container::multiPut),
                InstanceMethod("createIndex", &Container::createIndex),
                InstanceMethod("dropIndex", &Container::dropIndex),
                InstanceMethod("flush", &Container::flush),
                InstanceMethod("abort", &Container::abort),
                InstanceMethod("commit", &Container::commit),
                InstanceMethod("setAutoCommit", &Container::setAutoCommit),
                InstanceMethod("remove", &Container::remove),
                InstanceAccessor("type", &Container::getType, nullptr)
            });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Container", func);
    return exports;
}

static void freeMemoryContainer(GSContainerInfo** containerInfo,
        GSType** typeList) {
    if (*containerInfo) {
        int columnCount = static_cast<int>((*containerInfo)->columnCount);
        for (int i = 0; i < columnCount; i++) {
            if ((*containerInfo)->columnInfoList
                    && (*containerInfo)->columnInfoList[i].name) {
                delete[] (*containerInfo)->columnInfoList[i].name;
            }
        }
        if ((*containerInfo)->columnInfoList) {
            delete[] (*containerInfo)->columnInfoList;
        }
        if ((*containerInfo)->name) {
            delete[] (*containerInfo)->name;
        }
        delete (*containerInfo);
        *containerInfo = NULL;
    }
    if (*typeList) {
        delete[] *typeList;
        *typeList = NULL;
    }
}

Container::Container(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<Container>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 2 || !info[0].IsExternal() || !info[1].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", mContainer)
        return;
    }
    this->mContainer = info[0].As<Napi::External<GSContainer>>().Data();
    GSResult ret = gsCreateRowByContainer(mContainer, &mRow);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", mContainer)
        return;
    }

    GSContainerInfo* containerInfo =
            info[1].As<Napi::External<GSContainerInfo>>().Data();

    GSColumnInfo* columnInfoList;
    // Create local mContainerInfo: there is issue from C-API about using
    // share memory that make GSContainerInfo* pointer error in case :
    // create gsRow, get GSContainerInfo from gsRow, set field of gsRow
    try {
        mContainerInfo = new GSContainerInfo();
        (*mContainerInfo) = (*containerInfo);  // For normal data
        mContainerInfo->name = NULL;
        if (containerInfo->name) {
            mContainerInfo->name = Util::strdup(containerInfo->name);
        }

        columnInfoList = new GSColumnInfo[containerInfo->columnCount]();
        mContainerInfo->columnInfoList = columnInfoList;

        for (int i = 0; i < static_cast<int>(containerInfo->columnCount); i++) {
            columnInfoList[i].type = containerInfo->columnInfoList[i].type;
            if (containerInfo->columnInfoList[i].name) {
                columnInfoList[i].name =
                        Util::strdup(containerInfo->columnInfoList[i].name);
            } else {
                columnInfoList[i].name = NULL;
            }

            columnInfoList[i].indexTypeFlags =
                    containerInfo->columnInfoList[i].indexTypeFlags;
            columnInfoList[i].options =
                    containerInfo->columnInfoList[i].options;
        }

        mTypeList = new GSType[mContainerInfo->columnCount]();
    } catch (std::bad_alloc&) {
        // Memory allocation error
        freeMemoryContainer(&mContainerInfo, &mTypeList);
        THROW_EXCEPTION_WITH_STR(env, "Memory allocation error", mContainer)
        return;
    }

    mContainerInfo->timeSeriesProperties = NULL;
    mContainerInfo->triggerInfoList = NULL;
    mContainerInfo->dataAffinity = NULL;

    if (mTypeList && mContainerInfo->columnInfoList) {
        int columnCount = static_cast<int>(mContainerInfo->columnCount);
        for (int i = 0; i < columnCount; i++) {
            mTypeList[i] = mContainerInfo->columnInfoList[i].type;
        }
    }
}

Napi::Value Container::put(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsArray()) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mContainer)
    }
    Napi::Array rowWrapper = info[0].As<Napi::Array>();
    int colNum = mContainerInfo->columnCount;
    int length = rowWrapper.Length();

    if (length != colNum) {
        PROMISE_REJECT_WITH_STRING(deferred, env,
                "Num row is different with container info", mContainer)
    }

    Napi::Value fieldValue;
    for (int k = 0; k < length; k++) {
        GSType type = mTypeList[k];
        fieldValue = rowWrapper.Get(k);
        try {
            Util::toField(env, &fieldValue, mRow, k, type);
        } catch (const Napi::Error &e) {
            PROMISE_REJECT_WITH_ERROR(deferred, e)
        }
    }

    GSBool bExists;
    GSResult ret = gsPutRow(mContainer, NULL, mRow, &bExists);

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }
    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Container::query(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsString()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", mContainer)
        return env.Null();
    }
    std::string queryStr = info[0].As<Napi::String>().ToString().Utf8Value();

    GSQuery *pQuery;
    GSResult ret = gsQuery(mContainer, queryStr.c_str(), &pQuery);

    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mContainer)
        return env.Null();
    }

    // Create new Store object
    Napi::EscapableHandleScope scope(env);
    auto queryPtr = Napi::External<GSQuery>::New(env, pQuery);
    auto containerInfoPtr = Napi::External<GSContainerInfo>::New(env,
            mContainerInfo);
    auto gsRowPtr = Napi::External<GSRow>::New(env, mRow);

    return scope.Escape(Query::constructor.New( { queryPtr, containerInfoPtr,
            gsRowPtr })).ToObject();
}

Napi::Value Container::get(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mContainer)
    }
    Field field;
    GSType type = mTypeList[0];
    Napi::Value fieldValue = info[0].As<Napi::Value>();
    if (!(type == GS_TYPE_STRING || type == GS_TYPE_INTEGER
            || type == GS_TYPE_LONG || type == GS_TYPE_TIMESTAMP)) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Invalid key type",
                mContainer)
    }

    GSBool exists;
    GSResult ret;
    void *key = NULL;
    int32_t tmpIntValue;
    int64_t tmpLongValue;
    std::string tmpString;
    GSTimestamp tmpTimestampValue;
    const GSChar* rowkeyPtr;

    switch (type) {
    case GS_TYPE_STRING: {
        if (mContainerInfo->columnInfoList[0].type != GS_TYPE_STRING
                || !info[0].IsString()) {
            PROMISE_REJECT_WITH_STRING(deferred, env,
                    "wrong type of rowKey string", mContainer)
        }

        tmpString = fieldValue.ToString().Utf8Value();
        rowkeyPtr = tmpString.c_str();
        key = &rowkeyPtr;
        break;
    }
    case GS_TYPE_INTEGER: {
        if (mContainerInfo->columnInfoList[0].type != GS_TYPE_INTEGER
                || !info[0].IsNumber()) {
            PROMISE_REJECT_WITH_STRING(deferred, env,
                    "wrong type of rowKey integer", mContainer)
        }
        tmpIntValue = fieldValue.ToNumber().Int32Value();
        key = reinterpret_cast<void*>(&tmpIntValue);
        break;
    }
    case GS_TYPE_LONG:
        if (mContainerInfo->columnInfoList[0].type != GS_TYPE_LONG
                || !info[0].IsNumber()) {
            PROMISE_REJECT_WITH_STRING(deferred, env,
                    "wrong type of rowKey long", mContainer)
        }
        tmpLongValue = fieldValue.ToNumber().Int64Value();
        key = reinterpret_cast<void*>(&tmpLongValue);
        break;
    case GS_TYPE_TIMESTAMP:
        if (mContainerInfo->columnInfoList[0].type != GS_TYPE_TIMESTAMP) {
            PROMISE_REJECT_WITH_STRING(deferred, env,
                    "wrong type of rowKey timestamp", mContainer)
        }
        try {
            tmpTimestampValue = Util::toGsTimestamp(env, &fieldValue);
        } catch (const Napi::Error &e) {
            PROMISE_REJECT_WITH_ERROR(deferred, e)
        }
        key = reinterpret_cast<void*>(&tmpTimestampValue);
        break;
    default:
        PROMISE_REJECT_WITH_STRING(deferred, env,
                "wrong type of rowKey field", mContainer)
    }
    ret = gsGetRow(mContainer, key, mRow, &exists);
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }
    if (exists != GS_TRUE) {
        deferred.Resolve(env.Null());
        return deferred.Promise();
    }

    Napi::Value outputWrapper;
    // Get row data
    try {
        outputWrapper = Util::fromRow(
                env, mRow, mContainerInfo->columnCount, mTypeList);
    } catch (const Napi::Error &e) {
        PROMISE_REJECT_WITH_ERROR(deferred, e)
    }
    deferred.Resolve(outputWrapper);
    return deferred.Promise();
}

Napi::Value Container::queryByTimeSeriesRange(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (info.Length() != 2) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", mContainer)
    }
    Field field;
    Napi::Value startValue = info[0].As<Napi::Value>();
    Napi::Value endValue = info[1].As<Napi::Value>();

    GSResult ret;
    GSQuery* pQuery = NULL;
    GSTimestamp startTimestampValue = Util::toGsTimestamp(env, &startValue);
    GSTimestamp endTimestampValue = Util::toGsTimestamp(env, &endValue);

    ret = gsQueryByTimeSeriesRange(mContainer, startTimestampValue,
        endTimestampValue, &pQuery);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mContainer)
    }

    // Create new Store object
    Napi::EscapableHandleScope scope(env);
    auto queryPtr = Napi::External<GSQuery>::New(env, pQuery);
    auto containerInfoPtr = Napi::External<GSContainerInfo>::New(env,
            mContainerInfo);
    auto gsRowPtr = Napi::External<GSRow>::New(env, mRow);

    return scope.Escape(Query::constructor.New( { queryPtr, containerInfoPtr,
            gsRowPtr })).ToObject();
}

Container::~Container() {
    if (mRow != NULL) {
        gsCloseRow(&mRow);
        mRow = NULL;
    }
    GSBool allRelated = GS_FALSE;
    // Release container and all related resources
    if (mContainer != NULL) {
        gsCloseContainer(&mContainer, allRelated);
        mContainer = NULL;
    }
    freeMemoryContainer(&mContainerInfo, &mTypeList);
}

static void freeDataMultiPut(GSRow** listRowdata, int rowCount) {
    if (listRowdata) {
        for (int rowNum = 0; rowNum < rowCount; rowNum++) {
            gsCloseRow(&listRowdata[rowNum]);
        }
        delete[] listRowdata;
    }
}

Napi::Value Container::multiPut(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsArray()) {
        PROMISE_REJECT_WITH_STRING(deferred, env,
                "Expected array of array as input", mContainer)
    }

    Napi::Array rowArrayWrapper = info[0].As<Napi::Array>();
    int rowCount = rowArrayWrapper.Length();
    for (int i = 0; i < rowCount; i++) {
        if (!(((Napi::Value) rowArrayWrapper[i]).IsArray())) {
            // throw error
            PROMISE_REJECT_WITH_STRING(deferred, env,
                    "Expected array of array as input", mContainer)
        }
    }
    GSRow **listRowdata;
    if (rowCount == 0) {
        deferred.Resolve(env.Null());
        return deferred.Promise();
    }

    try {
        listRowdata = new GSRow*[rowCount]();
    } catch (std::bad_alloc&) {
        PROMISE_REJECT_WITH_STRING(
                deferred, env, "Memory allocation error", mContainer)
    }
    int length;
    GSResult ret;
    Napi::Value fieldValue;
    for (int i = 0; i < rowCount; i++) {
        Napi::Array rowWrapper = rowArrayWrapper.Get(i).As<Napi::Array>();
        length = rowWrapper.Length();
        if (length != static_cast<int>(mContainerInfo->columnCount)) {
            freeDataMultiPut(listRowdata, rowCount);
            PROMISE_REJECT_WITH_STRING(deferred, env,
                    "Num row is different with container info", mContainer)
        }
        ret = gsCreateRowByContainer(mContainer, &listRowdata[i]);
        if (!GS_SUCCEEDED(ret)) {
            freeDataMultiPut(listRowdata, rowCount);
            PROMISE_REJECT_WITH_STRING(deferred, env,
                    "Can't create GSRow", mContainer)
        }
        for (int k = 0; k < length; k++) {
            GSType type = mTypeList[k];
            fieldValue = rowWrapper.Get(k);
            try {
                Util::toField(env, &fieldValue, listRowdata[i], k, type);
            } catch(const Napi::Error& e) {
                freeDataMultiPut(listRowdata, rowCount);
                PROMISE_REJECT_WITH_ERROR(deferred, e);
            }
        }
    }

    GSBool bExists;
    // Data for each container
    ret = gsPutMultipleRows(mContainer, (const void * const *) listRowdata,
            rowCount, &bExists);

    freeDataMultiPut(listRowdata, rowCount);

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }

    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Container::createIndex(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsObject()) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mContainer)
    }

    Napi::Object input = info[0].As<Napi::Object>();

    std::string columnName;
    REQUIRE_MEMBER_STRING(columnName, "columnName", input, env, deferred)
    GSIndexTypeFlags indexType = GS_INDEX_FLAG_DEFAULT;
    OPTIONAL_MEMBER_INT32(indexType, "indexType", input)
    std::string name;
    OPTIONAL_MEMBER_STRING(name, "name", input)

    GSResult ret = GS_RESULT_OK;
    if (name.empty()) {
        ret = gsCreateIndex(mContainer, columnName.c_str(), indexType);
    } else {
        GSIndexInfo indexInfo = GS_INDEX_INFO_INITIALIZER;
        indexInfo.name = name.c_str();
        indexInfo.type = indexType;
        indexInfo.columnName = columnName.c_str();
        ret = gsCreateIndexDetail(mContainer, &indexInfo);
    }

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }

    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Container::dropIndex(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsObject()) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mContainer)
    }

    Napi::Object input = info[0].As<Napi::Object>();

    std::string columnName;
    REQUIRE_MEMBER_STRING(columnName, "columnName", input, env, deferred)

    GSIndexTypeFlags indexType = GS_INDEX_FLAG_DEFAULT;
    OPTIONAL_MEMBER_INT32(indexType, "indexType", input)

    std::string name;
    OPTIONAL_MEMBER_STRING(name, "name", input)

    GSResult ret = GS_RESULT_OK;

    if (name.empty()) {
        ret = gsDropIndex(mContainer, columnName.c_str(), indexType);
    } else {
        GSIndexInfo indexInfo = GS_INDEX_INFO_INITIALIZER;
        indexInfo.name = name.c_str();
        indexInfo.type = indexType;
        indexInfo.columnName = columnName.c_str();
        ret = gsDropIndexDetail(mContainer, &indexInfo);
    }

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }
    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Container::flush(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 0) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mContainer)
    }

    GSResult ret = gsFlush(mContainer);

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }

    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Container::abort(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 0) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mContainer)
    }

    GSResult ret = gsAbort(mContainer);

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }

    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Container::commit(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 0) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mContainer)
    }

    GSResult ret = gsCommit(mContainer);
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }

    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Container::setAutoCommit(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsBoolean()) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mContainer)
    }
    bool enabled = info[0].As<Napi::Boolean>();

    GSBool gsEnabled;
    gsEnabled = (enabled == true ? GS_TRUE : GS_FALSE);
    GSResult ret = gsSetAutoCommit(mContainer, gsEnabled);
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }

    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Container::remove(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mContainer)
    }

    Field field;
    GSType type = mTypeList[0];
    Napi::Value fieldValue = info[0].As<Napi::Value>();

    GSBool exists = GS_FALSE;
    GSResult ret;

    if (type == GS_TYPE_NULL) {
        ret = gsDeleteRow(mContainer, NULL, &exists);
    } else {
        switch (type) {
        case GS_TYPE_STRING: {
            GSChar* rowkeyPtr = const_cast<GSChar*> (fieldValue.
                                ToString().Utf8Value().c_str());
            const void * key = reinterpret_cast<void*>(&rowkeyPtr);
            ret = gsDeleteRow(mContainer,
                    key, &exists);
            break;
            }
        case GS_TYPE_INTEGER: {
            int tmpIntValue = fieldValue.ToNumber().Int32Value();
            ret = gsDeleteRow(mContainer, &tmpIntValue, &exists);
            break;
        }
        case GS_TYPE_LONG: {
            int64_t tmpLongValue = fieldValue.ToNumber().Int64Value();
            ret = gsDeleteRow(mContainer, &tmpLongValue, &exists);
            break;
        }
        case GS_TYPE_TIMESTAMP: {
            GSTimestamp tmpTimestampValue =
                    Util::toGsTimestamp(env, &fieldValue);
            ret = gsDeleteRow(mContainer, &tmpTimestampValue, &exists);
            break;
        }

        default:
            PROMISE_REJECT_WITH_STRING(deferred, env,
                    "wrong type of rowKey field", mContainer)
        }
    }

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mContainer)
    }

    if (!exists) {
        PROMISE_REJECT_WITH_STRING(
                deferred, env, "Row is not existing", mContainer)
    }
    deferred.Resolve(Napi::Boolean::New(env, ret));
    return deferred.Promise();
}

Napi::Value Container::getType(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    return Napi::Number::New(env, mContainerInfo->type);
}

}  // namespace griddb

