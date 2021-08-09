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

#include "Store.h"
#include <string>
#include <map>
#include <vector>

namespace griddb {

Napi::FunctionReference Store::constructor;

Napi::Object Store::init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "Store", {
            InstanceMethod("putContainer",
                &Store::putContainer),
            InstanceMethod(
                "dropContainer", &Store::dropContainer),
            InstanceMethod(
                "getContainer", &Store::getContainer),
            InstanceMethod(
                "getContainerInfo", &Store::getContainerInfo),
            InstanceMethod(
                "multiPut", &Store::multiPut),
            InstanceMethod(
                "multiGet", &Store::multiGet),
            InstanceMethod(
                "createRowKeyPredicate", &Store::createRowKeyPredicate),
            InstanceMethod(
                "fetchAll", &Store::fetchAll),
            InstanceAccessor("partitionController",
                &Store::getPartitionController,
                &Store::setReadonlyAttribute)
            });
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Store", func);
    return exports;
}

Store::Store(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<Store>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsExternal()) {
        // Throw error
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments", NULL)
        return;
    }

    this->mStore = info[0].As<Napi::External<GSGridStore>>().Data();
}

Napi::Value Store::putContainer(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    int length = info.Length();
    bool modifiable = false;
    ContainerInfo *containerInfo = NULL;
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    switch (length) {
    case 1:
        if (!info[0].IsObject()) {
            PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
        }
        containerInfo =
                    Napi::ObjectWrap<ContainerInfo>::Unwrap(
                            info[0].As<Napi::Object>());
        break;
    case 2:
        if (!info[0].IsObject() || !info[1].IsBoolean()) {
            PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
        }
        containerInfo =
                    Napi::ObjectWrap<ContainerInfo>::Unwrap(
                            info[0].As<Napi::Object>());

        modifiable = info[1].As<Napi::Boolean>().ToBoolean();
        break;
    default:
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }

    // Get Container information
    GSContainerInfo* gsInfo = containerInfo->gs_info();
    GSContainer* pContainer = NULL;
    // Create new gsContainer
    GSResult ret = gsPutContainerGeneral(
            mStore, gsInfo->name, gsInfo, modifiable, &pContainer);
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }

    // Create new Container object
    Napi::EscapableHandleScope scope(env);
    auto containerPtr = Napi::External<GSContainer>::New(env, pContainer);
    auto containerInfoPtr = Napi::External<GSContainerInfo >::New(env, gsInfo);
    Napi::Value containerWrapper;
    containerWrapper = scope.Escape(Container::constructor.New(
            {containerPtr, containerInfoPtr})).ToObject();
    // Return promise object
    deferred.Resolve(containerWrapper);
    return deferred.Promise();
}

Napi::Value Store::dropContainer(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsString()) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }
    std::string name = info[0].As<Napi::String>().Utf8Value();
    GSResult ret = gsDropContainer(mStore, name.c_str());

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }

    // Return promise object
    deferred.Resolve(env.Null());
    return deferred.Promise();
}

Napi::Value Store::getContainer(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsString()) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }
    std::string name = info[0].As<Napi::String>().Utf8Value();

    GSContainer* pContainer;
    GSResult ret = gsGetContainerGeneral(mStore, name.c_str(), &pContainer);

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }
    if (pContainer == NULL) {
        deferred.Resolve(env.Null());
        return deferred.Promise();
    }
    GSContainerInfo containerInfo = GS_CONTAINER_INFO_INITIALIZER;
    GSChar bExists;
    ret = gsGetContainerInfo(mStore, name.c_str(), &containerInfo, &bExists);
    if (!GS_SUCCEEDED(ret)) {
        gsCloseContainer(&pContainer, GS_FALSE);
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }

    // Create new Container object
    Napi::EscapableHandleScope scope(env);
    auto containerPtr = Napi::External<GSContainer>::New(env, pContainer);
    auto containerInfoPtr =
            Napi::External<GSContainerInfo >::New(env, &containerInfo);
    Napi::Value containerWrapper;
    containerWrapper = scope.Escape(Container::constructor.New({ containerPtr,
            containerInfoPtr })).ToObject();

    // Return promise object
    deferred.Resolve(containerWrapper);
    return deferred.Promise();
}

Store::~Store() {
    if (mStore != NULL) {
        gsCloseGridStore(&mStore, GS_TRUE);
        mStore = NULL;
    }
}

Napi::Value Store::getContainerInfo(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsString()) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }
    std::string name = info[0].As<Napi::String>().Utf8Value();

    GSContainerInfo gsContainerInfo = GS_CONTAINER_INFO_INITIALIZER;
    GSChar bExists;
    GSResult ret = gsGetContainerInfo(
            mStore, name.c_str(), &gsContainerInfo, &bExists);

    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }
    if (bExists == false) {
        deferred.Resolve(env.Null());
        return deferred.Promise();
    }
    // Create new ContainerInfo object
    Napi::EscapableHandleScope scope(env);
    auto containerInfoPtr = Napi::External<GSContainerInfo>::New(env,
            &gsContainerInfo);
    Napi::Value containerInfoWrapper = scope.Escape(
            ContainerInfo::constructor.New({containerInfoPtr})).ToObject();
    // Return promise object
    deferred.Resolve(containerInfoWrapper);
    return deferred.Promise();
}

Napi::Value Store::getPartitionController(
        const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    GSPartitionController* partitionController;

    GSResult ret = gsGetPartitionController(mStore, &partitionController);

    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mStore)
        return env.Null();
    }

    // Create new PartitionController object
    Napi::EscapableHandleScope scope(env);
    auto controllerPtr = Napi::External<GSPartitionController>::New(info.Env(),
            partitionController);
    return scope.Escape(PartitionController::constructor.New( {
        controllerPtr })).ToObject();
}

static void freeMemoryDataMultiPut(const char **listContainerName,
            int *listRowContainerCount, GSContainerRowEntry *entryList,
            GSRow* **allRowList, int containerCount) {
    // Free memory
    if (listContainerName) {
        for (int i = 0; i < containerCount; i++) {
            if (listContainerName[i]) {
                gsCloseContainer(reinterpret_cast<GSContainer **>(
                    const_cast<char**>(&listContainerName[i])), GS_FALSE);
                delete[] listContainerName[i];
            }
        }
        delete listContainerName;
    }
    if (listRowContainerCount) {
        delete listRowContainerCount;
    }
    if (allRowList) {
        for (int i = 0; i < containerCount; i++) {
            if (allRowList[i]) {
                for (int j = 0; j < static_cast<int>(entryList[i].rowCount);
                        j++) {
                    if (allRowList[i][j]) {
                        gsCloseRow(&allRowList[i][j]);
                    }
                }
                delete [] allRowList[i];
            }
        }
        delete [] allRowList;
    }
    if (entryList) {
        delete[] entryList;
    }
}

Napi::Value Store::multiPut(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    size_t containerCount;
    GSResult ret;
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsObject()) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }

    Napi::Object objNapi = info[0].As<Napi::Object>();
    Napi::Array objProp = objNapi.GetPropertyNames();
    containerCount = objProp.Length();
    GSContainer *containerPtr;

    const char **listContainerName = NULL;
    int *listRowContainerCount = NULL;
    GSContainerRowEntry *entryList = NULL;
    GSRow* **allRowList = NULL;
    GSBool bExists;
    try {
        listContainerName = new const char*[containerCount]();
        listRowContainerCount = new int[containerCount]();
        entryList = new GSContainerRowEntry[containerCount]();
        allRowList = new GSRow**[containerCount]();
    } catch(std::bad_alloc& ba) {
        freeMemoryDataMultiPut(listContainerName, listRowContainerCount,
                    entryList, allRowList, containerCount);
        PROMISE_REJECT_WITH_STRING(deferred, env, "Memory allocation error",
                mStore)
    }
    for (int i = 0; i < static_cast<int>(containerCount); i++) {
        entryList[i] = GS_CONTAINER_ROW_ENTRY_INITIALIZER;
        Napi::Value value = objProp[i];
        if (!value.IsString()) {
            freeMemoryDataMultiPut(listContainerName, listRowContainerCount,
                        entryList, allRowList, containerCount);
            PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments\n",
                    mStore)
        }
        std::string strContainerName = value.ToString().Utf8Value();
        Napi::Array arrOfContainer = objNapi.Get(
                strContainerName.c_str()).As<Napi::Array>();
        listContainerName[i] = Util::strdup(strContainerName.c_str());

        listRowContainerCount[i] = arrOfContainer.Length();
        const int rowCount = arrOfContainer.Length();
        try {
            allRowList[i] = new GSRow*[rowCount]();
        } catch (std::bad_alloc& ba) {
            freeMemoryDataMultiPut(listContainerName, listRowContainerCount,
                        entryList, allRowList, containerCount);
            PROMISE_REJECT_WITH_STRING(deferred, env,
                    "Memory allocation error", mStore)
        }
        ret = gsGetContainerGeneral(mStore, listContainerName[i],
                &containerPtr);
        if (!GS_SUCCEEDED(ret) || containerPtr == NULL) {
            freeMemoryDataMultiPut(listContainerName, listRowContainerCount,
                        entryList, allRowList, containerCount);
            PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
        }

        GSContainerInfo containerInfo = GS_CONTAINER_INFO_INITIALIZER;
        ret = gsGetContainerInfo(mStore, listContainerName[i], &containerInfo,
                &bExists);
        if (!GS_SUCCEEDED(ret) || bExists == GS_FALSE) {
            freeMemoryDataMultiPut(listContainerName, listRowContainerCount,
                        entryList, allRowList, containerCount);
            PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
        }

        int* typeList = new int[containerInfo.columnCount]();
        for (int t = 0; t < static_cast<int>(containerInfo.columnCount); t++) {
            typeList[t] = containerInfo.columnInfoList[t].type;
        }
        for (int k = 0; k < listRowContainerCount[i]; k++) {
            GSRow *row;
            ret = gsCreateRowByContainer(containerPtr, &row);
            if (!GS_SUCCEEDED(ret)) {
                freeMemoryDataMultiPut(listContainerName,
                        listRowContainerCount, entryList, allRowList,
                        containerCount);
                PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
            }
            Napi::Value oneValue = arrOfContainer[k];
            Napi::Array arrayOneRow = oneValue.As<Napi::Array>();
            if (!oneValue.IsArray()) {
                delete typeList;
                PROMISE_REJECT_WITH_STRING(deferred, env,
                    "Expected an array as rowList", mStore)
            }
            for (int j = 0; j < static_cast<int>(arrayOneRow.Length()); j++) {
                Napi::Value oneVal = arrayOneRow[j];
                try {
                    Util::toField(env, &oneVal, row, j, typeList[j]);
                } catch (const Napi::Error &e) {
                    delete typeList;
                    freeMemoryDataMultiPut(listContainerName,
                            listRowContainerCount, entryList, allRowList,
                            containerCount);
                    PROMISE_REJECT_WITH_ERROR(deferred, e)
                }
            }
            allRowList[i][k] = row;
        }
        delete typeList;
        entryList[i].containerName = listContainerName[i];
        entryList[i].rowCount = listRowContainerCount[i];
        entryList[i].rowList = (void* const*)allRowList[i];
    }
    ret = gsPutMultipleContainerRows(mStore, entryList, containerCount);
    // Free memory
    freeMemoryDataMultiPut(listContainerName, listRowContainerCount,
                entryList, allRowList, containerCount);
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }
    // Return promise object
    deferred.Resolve(env.Null());
    return deferred.Promise();
}

// Create RowKey Predicate
Napi::Value Store::createRowKeyPredicate(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsNumber()) {
        THROW_EXCEPTION_WITH_STR(env, "Wrong arguments\n", mStore)
        return env.Null();
    }

    GSType type = info[0].As<Napi::Number>().Int32Value();
    GSRowKeyPredicate* predicate;
    GSResult ret = gsCreateRowKeyPredicate(mStore, type, &predicate);
    if (!GS_SUCCEEDED(ret)) {
        THROW_EXCEPTION_WITH_CODE(env, ret, mStore)
        return env.Null();
    }
    Napi::EscapableHandleScope scope(env);
    auto predicateNode = Napi::External<
                GSRowKeyPredicate>::New(env, predicate);
    auto typeExt = Napi::External<GSType>::New(env, &type);
    return scope.Escape( RowKeyPredicate::constructor.New(
                {predicateNode, typeExt})).ToObject();
}

// Free memory for function setMultiContainerNumList
static void freeMemoryDataSetMulti(
            const GSRowKeyPredicateEntry* const *predicate, int **colNumList,
            GSType*** typeList, int length) {
    if (predicate) {
        delete[] predicate;
    }
    if (colNumList) {
        for (int i = 0; i < length; i++) {
            if (colNumList[i]) {
                delete [] colNumList[i];
            }
        }
        delete[] colNumList;
    }
    if (typeList) {
        for (int i = 0; i < length; i++) {
            if (typeList[i]) {
                delete [] typeList[i];
            }
        }
        delete[] typeList;
    }
}

// Function set multi montainer numList support for method multiGet
static bool setMultiContainerNumList(Napi::Env env, GSGridStore *mStore,
                const GSRowKeyPredicateEntry* const * predicateList,
                int length, int **colNumList, GSType*** typeList) {
    GSResult ret;
    GSBool bExists;
    GSContainerInfo containerInfo;
    try {
        *colNumList = new int[length]();
        *typeList = new GSType*[length]();
    } catch (std::bad_alloc& ba) {
        freeMemoryDataSetMulti(predicateList, colNumList, typeList, length);
        return false;
    }
    for (int i = 0; i < length; i++) {
        ret = gsGetContainerInfo(mStore, (*predicateList)[i].containerName,
                &containerInfo, &bExists);
        if (!GS_SUCCEEDED(ret)) {
            freeMemoryDataSetMulti(predicateList, colNumList, typeList,
                    length);
            THROW_EXCEPTION_WITH_CODE(env, ret, mStore)
            return false;
        }
        (*colNumList)[i] = containerInfo.columnCount;
        try {
            (*typeList)[i] = new GSType[(*colNumList)[i]]();
        } catch (std::bad_alloc& ba) {
            freeMemoryDataSetMulti(predicateList, colNumList, typeList,
                    length);
            return false;
        }
        for (int j = 0; j < (*colNumList)[i]; j++) {
            (*typeList)[i][j] = containerInfo.columnInfoList[j].type;
        }
    }
    return true;
}

// Free memory for method multiGet
static void freeMemoryDataMultiGet(GSRowKeyPredicateEntry *predicate,
            int *colNumList, GSType** typeList, int containerCount,
            GSContainerRowEntry **entryList) {
    if (predicate) {
        for (int i = 0; i < containerCount; i++) {
            if (predicate[i].containerName) {
                delete [] predicate[i].containerName;
            }
        }
        delete[] predicate;
    }
    if (colNumList) {
        delete [] colNumList;
    }
    if (typeList) {
        for (int i = 0; i < containerCount; i++) {
            delete [] typeList[i];
        }
        delete [] typeList;
    }
    if (entryList) {
        GSRow* row;
        for (int i = 0; i < containerCount; i++) {
            for (int j = 0; j < static_cast<int>((*entryList)[i].rowCount);
                    j++) {
                row = reinterpret_cast<GSRow*>((*entryList)[i].rowList[j]);
                gsCloseRow(&row);
            }
        }
    }
}

// Multi get container
Napi::Value Store::multiGet(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 || !info[0].IsObject()) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }
    const GSContainerRowEntry *outEntryList;
    size_t outEntryCount;
    Napi::Object objNapi = info[0].As<Napi::Object>();
    Napi::Array objProp = objNapi.GetPropertyNames();
    int32_t containerCount = objProp.Length();
    GSRowKeyPredicateEntry *predEntryValueList;
    std::map<std::string, std::vector<int> > dict;
    try {
        predEntryValueList = new GSRowKeyPredicateEntry[containerCount]();
    } catch (std::bad_alloc& ba) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Memory allocation error",
                mStore)
    }
    const GSRowKeyPredicateEntry *const * predicateList = &predEntryValueList;
    for (int i = 0; i < containerCount; i++) {
        Napi::Value value = objProp[i];
        std::string strContainerName = value.ToString().Utf8Value();
        Napi::Value valProp = objNapi.Get(strContainerName.c_str());
        RowKeyPredicate *predicate = Napi::ObjectWrap<RowKeyPredicate>
                    ::Unwrap(valProp.As<Napi::Object>());
        predEntryValueList[i].containerName =
                    Util::strdup(strContainerName.c_str());
        predEntryValueList[i].predicate = predicate->getPredicate();
    }
    int *colNumList;
    GSType** typeList;
    bool setNumList = setMultiContainerNumList(env, mStore, predicateList,
                containerCount, &colNumList, &typeList);
    if (!setNumList) {
        freeMemoryDataMultiGet(predEntryValueList, colNumList, typeList,
                    containerCount,
                    const_cast<GSContainerRowEntry**>(&outEntryList));
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }

    // Save type list befor call C-API getMulti
    for (int i = 0; i < containerCount; i++) {
        Napi::Value value = objProp[i];
        std::string strContainerName = value.ToString().Utf8Value();
        for (int j = 0 ; j < colNumList[i]; j++) {
            dict[strContainerName].push_back(typeList[i][j]);
        }
    }
    GSResult ret = gsGetMultipleContainerRows(mStore, predicateList,
                containerCount, &outEntryList, &outEntryCount);
    if (!GS_SUCCEEDED(ret)) {
        freeMemoryDataMultiGet(predEntryValueList, colNumList, typeList,
                    containerCount,
                    const_cast<GSContainerRowEntry**>(&outEntryList));
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }

    // Loop get data
    Napi::Object objResult = Napi::Object::New(env);
    for (int i = 0; i  < static_cast<int>(outEntryCount); i++) {
        Napi::Array tmpArr = Napi::Array::New(env);
        std::vector<int> vectorType = dict.at(outEntryList[i].containerName);
        for (int j = 0 ; j < static_cast<int>(outEntryList[i].rowCount);
                j++) {
            Napi::Array tmpArrChild  = Napi::Array::New(env);
            for (int q = 0; q < static_cast<int>(vectorType.size()); q++) {
                try {
                    tmpArrChild.Set(q, Util::fromField(env,
                        reinterpret_cast<GSRow *>(outEntryList[i].rowList[j]),
                        q, vectorType.at(q)));
                } catch (const Napi::Error &e) {
                    freeMemoryDataMultiGet(predEntryValueList, colNumList,
                        typeList, containerCount,
                        const_cast<GSContainerRowEntry**>(&outEntryList));
                    PROMISE_REJECT_WITH_ERROR(deferred, e)
                }
            }
            tmpArr.Set(j, tmpArrChild);
        }
        objResult.Set(outEntryList[i].containerName, tmpArr);
    }
    // Free memory
    freeMemoryDataMultiGet(predEntryValueList, colNumList, typeList,
                containerCount,
                const_cast<GSContainerRowEntry**>(&outEntryList));
    // Return promise object
    deferred.Resolve(objResult);
    return deferred.Promise();
}

Napi::Value Store::fetchAll(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    if (info.Length() != 1 && !(info[0].IsArray() || info[0].IsNull())) {
        // Throw error
        PROMISE_REJECT_WITH_STRING(deferred, env, "Wrong arguments", mStore)
    }

    GSResult ret;
    size_t queryCount;
    GSQuery **queryList;
    if (!info[0].IsNull()) {
        queryCount = info[0].As<Napi::Array>().Length();
    } else {
        queryCount = 0;
    }
    try {
        queryList = new GSQuery*[queryCount];
    } catch (std::bad_alloc& ba) {
        PROMISE_REJECT_WITH_STRING(deferred, env, "Memory allocation error",
                mStore)
    }

    Napi::Array tmpArr = info[0].As<Napi::Array>();
    for (int i = 0; i < static_cast<int>(queryCount); i++) {
        Napi::Value tmpVal = tmpArr[i];
        Query *query = Napi::ObjectWrap<Query>
                ::Unwrap(tmpVal.As<Napi::Object>());
        queryList[i] = query->gsPtr();
    }
    ret = gsFetchAll(mStore, (GSQuery* const*)queryList, queryCount);
    // Free memory
    delete [] queryList;
    if (!GS_SUCCEEDED(ret)) {
        PROMISE_REJECT_WITH_ERROR_CODE(deferred, env, ret, mStore)
    }
    deferred.Resolve(env.Null());
    return deferred.Promise();
}

void Store::setReadonlyAttribute(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    Napi::Env env = info.Env();
    THROW_EXCEPTION_WITH_STR(env, "Can't set read only attribute", mStore)
}

}  // namespace griddb

