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

#include <string>
#include "ContainerInfo.h"

namespace griddb {

Napi::FunctionReference ContainerInfo::constructor;

Napi::Object ContainerInfo::init(const Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "ContainerInfo", {
            InstanceAccessor("name",
                    &ContainerInfo::getName,
                    &ContainerInfo::setName),
            InstanceAccessor("type",
                    &ContainerInfo::getType,
                    &ContainerInfo::setType),
            InstanceAccessor("rowKey",
                    &ContainerInfo::getRowKeyAssigned,
                    &ContainerInfo::setRowKeyAssigned),
            InstanceAccessor("columnInfoList",
                    &ContainerInfo::getColumnInfoList,
                    &ContainerInfo::setColumnInfoList),
            InstanceAccessor("expiration",
                    &ContainerInfo::getExpiration,
                    &ContainerInfo::setExpiration),
        });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("ContainerInfo", func);
    return exports;
}

ContainerInfo::ContainerInfo(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<ContainerInfo>(info),
        mContainerInfo(GS_CONTAINER_INFO_INITIALIZER), mExpInfo(NULL) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    int length = info.Length();
    if (length == 1 && info[0].IsExternal()) {
        // Use only for method : Store.getContainerInfo()
        GSContainerInfo *containerInfo = (info[0].As<
                Napi::External<GSContainerInfo>>().Data());
        init(env, containerInfo->name, containerInfo->type,
                containerInfo->columnInfoList, containerInfo->columnCount,
                containerInfo->rowKeyAssigned, NULL);
        // Assign values from argument to mContainer
        GSTimeSeriesProperties *gsProps = NULL;
        GSTriggerInfo *triggerInfoList = NULL;

        try {
            if (containerInfo->timeSeriesProperties) {
                gsProps = new GSTimeSeriesProperties();
            }

            if (containerInfo->triggerInfoList) {
                triggerInfoList = new GSTriggerInfo();
            }

            if (containerInfo->dataAffinity) {
                mContainerInfo.dataAffinity = Util::strdup(
                        containerInfo->dataAffinity);
            } else {
                mContainerInfo.dataAffinity = NULL;
            }
        } catch (std::bad_alloc &ba) {
            // Case allocation memory error
            if (gsProps) {
                delete gsProps;
            }
            if (triggerInfoList) {
                delete triggerInfoList;
            }
            if (mContainerInfo.dataAffinity) {
                delete[] mContainerInfo.dataAffinity;
            }
            THROW_EXCEPTION_WITH_STR(env, "Memory allocation error", NULL)
            return;
        }

        if (containerInfo->timeSeriesProperties) {
            memcpy(gsProps, containerInfo->timeSeriesProperties,
                    sizeof(GSTimeSeriesProperties));
        }
        mContainerInfo.timeSeriesProperties = gsProps;

        if (containerInfo->triggerInfoList) {
            memcpy(triggerInfoList, containerInfo->triggerInfoList,
                    sizeof(GSTriggerInfo));
        }

        mContainerInfo.triggerInfoList = triggerInfoList;

        mContainerInfo.columnOrderIgnorable =
                containerInfo->columnOrderIgnorable;
        mContainerInfo.triggerInfoCount = containerInfo->triggerInfoCount;
        mColumnInfoList.columnInfo = NULL;
        mColumnInfoList.size = 0;
        return;
    } else if (length == 1  && info[0].IsObject()) {
        // Case create ContainerInfo : new griddb.ContainerInfo({..});
        Napi::Object obj = info[0].As<Napi::Object>();

        // Name attribute
        if (!obj.Has("name")) {
            THROW_EXCEPTION_WITH_STR(env, "Missing name attribute", NULL)
            return;
        }
        if (!obj.Get("name").IsString()) {
            THROW_EXCEPTION_WITH_STR(
                    env, "Name attribute should be string", NULL)
            return;
        }

        std::string name = obj.Get("name").ToString().Utf8Value();

        // Type attribute
        GSContainerType type = GS_CONTAINER_COLLECTION;

        if (obj.Has("type") && !obj.Get("type").IsNumber()) {
            THROW_EXCEPTION_WITH_STR(
                    env, "Attribute type should be number", NULL)
            return;
        }
        type = obj.Get("type").ToNumber().Int32Value();

        // RowKey attribute
        bool rowKey = true;
        if (obj.Has("rowKey") && !obj.Get("rowKey").IsBoolean()) {
            THROW_EXCEPTION_WITH_STR(
                    env, "Attribute rowKey should be bool", NULL)
            return;
        }
        if (obj.Has("rowKey")) {
            rowKey = obj.Get("rowKey").ToBoolean();
        }

        // ExpirationInfo
        ExpirationInfo *expiration = NULL;
        if (obj.Has("expiration")) {
            expiration = Napi::ObjectWrap<ExpirationInfo>::Unwrap(
                    obj.Get("expiration").As<Napi::Object>());
        }

        // columnInfoList attribute
        if (!obj.Has("columnInfoList")) {
            THROW_EXCEPTION_WITH_STR(env,
                    "Missing columnInfoList attribute", NULL)
            return;
        }
        if (!obj.Get("columnInfoList").IsArray()) {
            THROW_EXCEPTION_WITH_STR(env,
                    "Attribute columnInfoList should be array", NULL)
            return;
        }
        Napi::Array columnInfoListArray =
                obj.Get("columnInfoList").As<Napi::Array>();

        int propsCount = columnInfoListArray.Length();
        GSColumnInfo *props = new GSColumnInfo[propsCount]();
        for (int i = 0; i < propsCount; i++) {
            if (!((Napi::Value) columnInfoListArray[i]).IsArray()) {
                // Free memory of props
                this->freeMemoryPropsList(props, propsCount);
                THROW_EXCEPTION_WITH_STR(env,
                        "Element of columnInfoList should be array", NULL)
                return;
            }
            Napi::Array fieldInfo = ((Napi::Value) columnInfoListArray[i]).As<
                    Napi::Array>();
            if (fieldInfo.Length() < 2 || fieldInfo.Length() > 3) {
                // Free memory of props
                this->freeMemoryPropsList(props, propsCount);
                THROW_EXCEPTION_WITH_STR(env,
                        "Expected 2 or 3 elements for ColumnInfo property",
                        NULL)
                return;
            }

            bool firstFieldElementIsString =
                    ((Napi::Value) (fieldInfo["0"])).IsString();
            if (!firstFieldElementIsString) {
                this->freeMemoryPropsList(props, propsCount);
                THROW_EXCEPTION_WITH_STR(env,
                        "Expected 1st element of ColumnInfo property is string",
                        NULL)
                return;
            }
            props[i].name = Util::strdup(((Napi::Value) (fieldInfo["0"])).
                    ToString().Utf8Value().c_str());
            bool secondFieldElementIsNumber =
                    ((Napi::Value) (fieldInfo["1"])).IsNumber();
            if (!secondFieldElementIsNumber) {
                // Free memory of props
                this->freeMemoryPropsList(props, propsCount);
                THROW_EXCEPTION_WITH_STR(env,
                        "Expected 1st element of ColumnInfo property is string",
                        NULL)
                return;
            }

            props[i].type = ((Napi::Value) (fieldInfo[1])).
                    ToNumber().Int32Value();

            if (fieldInfo.Length() == 3) {
                bool thirdFieldElementIsNumber =
                        ((Napi::Value) (fieldInfo["2"])).IsNumber();
                if (!thirdFieldElementIsNumber) {
                    this->freeMemoryPropsList(props, propsCount);
                    THROW_EXCEPTION_WITH_STR(env,
                            "Expected Integer as type of Column options", NULL)
                    return;
                }
                props[i].options = ((Napi::Value) (fieldInfo["2"])).
                        ToNumber().Int32Value();
            }
        }

        try {
            this->init(env, name.c_str(), type, props, propsCount,
                    rowKey, expiration);
        } catch (const Napi::Error &e) {
            this->freeMemoryPropsList(props, propsCount);
            NAPI_THROW(e);
        }
        this->freeMemoryPropsList(props, propsCount);
    } else {
        THROW_EXCEPTION_WITH_STR(env, "Invalid input", NULL)
    }
}

/**
 * Initialize values of Container Info object
 */
void ContainerInfo::init(const Napi::Env &env, const GSChar *name,
        GSContainerType type, const GSColumnInfo *props, int propsCount,
        bool rowKeyAssigned, ExpirationInfo *expiration) {
    GSColumnInfo *columnInfoList = NULL;
    const GSChar *containerName = NULL;
    GSTimeSeriesProperties *timeProps = NULL;

    try {
        if (propsCount > 0 && props != NULL) {
            columnInfoList = new GSColumnInfo[propsCount]();
            // Copy memory of GSColumnInfo list
            memcpy(columnInfoList, props, propsCount * sizeof(GSColumnInfo));
            // Copy memory of columns name
            for (int i = 0; i < propsCount; i++) {
                if (props[i].name != NULL) {
                    columnInfoList[i].name = Util::strdup(props[i].name);
                } else {
                    columnInfoList[i].name = NULL;
                }
            }
        }

        if (expiration != NULL) {
            timeProps = new GSTimeSeriesProperties();
        }

        // Container name memory is copied via strdup function
        if (name != NULL) {
            containerName = Util::strdup(name);
        }
    } catch (std::bad_alloc &ba) {
        if (columnInfoList) {
            for (int i = 0; i < propsCount; i++) {
                if (columnInfoList[i].name) {
                    delete[] columnInfoList[i].name;
                }
            }
            delete[] columnInfoList;
        }
        if (containerName) {
            delete[] containerName;
        }
        if (timeProps) {
            delete timeProps;
        }
        THROW_EXCEPTION_WITH_STR(env, "Memory allocation error", NULL)
        return;
    }

    if (expiration != NULL) {
        memcpy(timeProps, expiration->gs_ts(), sizeof(GSTimeSeriesProperties));
    }

    mContainerInfo.name = containerName;
    mContainerInfo.type = type;
    mContainerInfo.columnCount = (size_t) propsCount;
    mContainerInfo.columnInfoList = columnInfoList;
    mContainerInfo.rowKeyAssigned = rowKeyAssigned;
    if (timeProps != NULL) {
        mContainerInfo.timeSeriesProperties = timeProps;
    }
    mExpInfo = NULL;
    mColumnInfoList.columnInfo = NULL;
    mColumnInfoList.size = 0;
}

void ContainerInfo::freeMemoryPropsList(GSColumnInfo *props,
        int propsCount) {
    if (props == NULL) {
        return;
    }
    for (int i = 0; i < propsCount; i++) {
        if (props[i].name) {
            delete[] props[i].name;
        }
    }
    delete[] props;
}

ContainerInfo::~ContainerInfo() {
    // Free memory for the copy of container name
    if (mContainerInfo.name) {
        delete[] mContainerInfo.name;
    }

    // Free memory for the copy of ColumnInfo list
    if (mContainerInfo.columnInfoList) {
        // Free memory of columns name
        for (int i = 0; i < static_cast<int>(mContainerInfo.columnCount); i++) {
            if (mContainerInfo.columnInfoList[i].name) {
                delete[] mContainerInfo.columnInfoList[i].name;
            }
        }
        delete[] mContainerInfo.columnInfoList;
    }

    // Free memory of TimeSeriesProperties if existed
    if (mContainerInfo.timeSeriesProperties) {
        delete mContainerInfo.timeSeriesProperties;
    }

    // Free memory of dataAffinity if existed
    if (mContainerInfo.dataAffinity) {
        delete[] mContainerInfo.dataAffinity;
    }

    // Free memory of triggerInfoList if existed
    if (mContainerInfo.triggerInfoList) {
        delete mContainerInfo.triggerInfoList;
    }
    if (mExpInfo != NULL) {
        delete mExpInfo;
    }
}

Napi::Value ContainerInfo::getName(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, mContainerInfo.name);
}

void ContainerInfo::setName(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    Napi::Env env = info.Env();
    if (!value.IsString()) {
        THROW_EXCEPTION_WITH_STR(env, "String expected", NULL)
        return;
    }
    std::string containerName = value.ToString().Utf8Value();

    if (mContainerInfo.name) {
        delete[] mContainerInfo.name;
    }

    mContainerInfo.name = Util::strdup(containerName.c_str());
}

Napi::Value ContainerInfo::getType(const Napi::CallbackInfo &info) {
    return Napi::Number::New(info.Env(), mContainerInfo.type);
}

void ContainerInfo::setType(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    Napi::Env env = info.Env();
    int length = info.Length();
    if (length != 1 || !info[0].IsNumber()) {
        THROW_EXCEPTION_WITH_STR(env, "Number expected", NULL)
        return;
    }
    GSContainerType containerType = info[0].As<Napi::Number>().Int32Value();
    mContainerInfo.type = containerType;
}

Napi::Value ContainerInfo::getRowKeyAssigned(
        const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, mContainerInfo.rowKeyAssigned);
}

void ContainerInfo::setRowKeyAssigned(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    Napi::Env env = info.Env();
    int length = info.Length();
    if (length <= 0 || !info[0].IsBoolean()) {
        THROW_EXCEPTION_WITH_STR(env, "Boolean expected", NULL)
        return;
    }
    bool rowkeyAssigned = info[0].As<Napi::Boolean>();
    mContainerInfo.rowKeyAssigned = (rowkeyAssigned == true)? GS_TRUE: GS_FALSE;
}

void ContainerInfo::setColumnInfoList(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    ColumnInfoList columnInfoList;

    Napi::Env env = info.Env();
    if (!value.IsArray()) {
        THROW_EXCEPTION_WITH_STR(env, "Expected array as input", NULL)
        return;
    }

    Napi::Array arr = value.As<Napi::Array>();
    size_t len = (size_t) arr.Length();
    if (len == 0) {
        THROW_EXCEPTION_WITH_STR(env, "Expected not empty array as input", NULL)
        return;
    }

    GSColumnInfo *containerInfo;

    try {
        containerInfo = new GSColumnInfo[len]();
    } catch (std::bad_alloc &ba) {
        THROW_EXCEPTION_WITH_STR(env, "Memory allocation error", NULL)
        return;
    }

    columnInfoList.columnInfo = containerInfo;
    columnInfoList.size = len;

    Napi::Array colInfo;
    Napi::Value key;
    Napi::Value value1;
    Napi::Value options;
    for (int i = 0; i < static_cast<int>(len); i++) {
        if (!(arr.Get(i)).IsArray()) {
            this->freeColumnInfoList(&columnInfoList);
            THROW_EXCEPTION_WITH_STR(env,
                    "Expected array property as ColumnInfo element", NULL)
            return;
        }
        colInfo = arr.Get(i).As<Napi::Array>();

        if (colInfo.Length() < 2) {
            this->freeColumnInfoList(&columnInfoList);
            THROW_EXCEPTION_WITH_STR(env,
                    "Expected at least two elements for ColumnInfo property",
                    NULL)
            return;
        }
        key = colInfo.Get("0");
        value1 = colInfo.Get("1");
        if (!key.IsString()) {
            this->freeColumnInfoList(&columnInfoList);
            THROW_EXCEPTION_WITH_STR(
                    env, "Expected string for column name", NULL)
            return;
        }
        if (!value1.IsNumber()) {
            this->freeColumnInfoList(&columnInfoList);
            THROW_EXCEPTION_WITH_STR(
                    env, "Expected Integer as type of Column type", NULL)
            return;
        }

        containerInfo[i].name = Util::strdup(
                key.As<Napi::String>().Utf8Value().c_str());
        containerInfo[i].type = value1.As<Napi::Number>().Int32Value();
        if (colInfo.Length() == 3) {
            options = colInfo.Get("2");
            if (!options.IsNumber()) {
                this->freeColumnInfoList(&columnInfoList);
                THROW_EXCEPTION_WITH_STR(env,
                        "Expected Integer as type of Column options", NULL)
                return;
            }

            containerInfo[i].options = options.As<Napi::Number>().Int32Value();
        }
        if (colInfo.Length() > 3) {
            this->freeColumnInfoList(&columnInfoList);
            THROW_EXCEPTION_WITH_STR(env,
                    "Expected at most 3 elements for ColumnInfo property", NULL)
            return;
        }
    }

    // Free current stored ColumnInfo list
    if (mContainerInfo.columnInfoList) {
        // Free memory of columns name
        for (int i = 0; i < static_cast<int>(mContainerInfo.columnCount); i++) {
            delete[] mContainerInfo.columnInfoList[i].name;
        }
        delete[] mContainerInfo.columnInfoList;
    }

    mContainerInfo.columnCount = columnInfoList.size;
    mContainerInfo.columnInfoList = NULL;

    if (columnInfoList.size == 0 || columnInfoList.columnInfo == NULL) {
        return;
    }

    GSColumnInfo *tmpColumnInfoList = NULL;
    try {
        tmpColumnInfoList = new GSColumnInfo[columnInfoList.size]();
    } catch (std::bad_alloc &ba) {
        THROW_EXCEPTION_WITH_STR(env, "Memory allocation error", NULL)
        return;
    }

    // Copy memory of GSColumnInfo list
    memcpy(tmpColumnInfoList, columnInfoList.columnInfo,
            columnInfoList.size * sizeof(GSColumnInfo));
    // Copy memory of columns name
    for (int i = 0; i < static_cast<int>(columnInfoList.size); i++) {
        if (columnInfoList.columnInfo[i].name) {
            try {
                tmpColumnInfoList[i].name = Util::strdup(
                        columnInfoList.columnInfo[i].name);
            } catch (std::bad_alloc &ba) {
                delete[] tmpColumnInfoList;
                tmpColumnInfoList = NULL;
                THROW_EXCEPTION_WITH_STR(env, "Memory allocation error", NULL)
                return;
            }
        } else {
            tmpColumnInfoList[i].name = NULL;
        }
    }
    mContainerInfo.columnInfoList = tmpColumnInfoList;

    this->freeColumnInfoList(&columnInfoList);
}

Napi::Value ContainerInfo::getColumnInfoList(
        const Napi::CallbackInfo &info) {
    mColumnInfoList.columnInfo =
            const_cast<GSColumnInfo*>(mContainerInfo.columnInfoList);
    mColumnInfoList.size = mContainerInfo.columnCount;

    ColumnInfoList columnInfoList = mColumnInfoList;
    Napi::Env env = info.Env();
    Napi::Array returnWrapper = Napi::Array::New(env);

    size_t len = columnInfoList.size;
    Napi::Array obj;

    if (len > 0) {
        obj = Napi::Array::New(env, len);
        for (int i = 0; i < static_cast<int>(len); i++) {
            Napi::Array prop = Napi::Array::New(env);
            prop.Set("0",
                    Napi::String::New(env, columnInfoList.columnInfo[i].name));
            prop.Set("1",
                    Napi::Number::New(env, columnInfoList.columnInfo[i].type));
            prop.Set("2",
                    Napi::Number::New(env,
                            columnInfoList.columnInfo[i].options));
            returnWrapper.Set(i, prop);
        }
    }
    return returnWrapper;
}

void ContainerInfo::freeColumnInfoList(
        ColumnInfoList *columnInfoList) {
    size_t len = columnInfoList->size;
    if (columnInfoList->columnInfo) {
        for (int i = 0; i < static_cast<int>(len); i++) {
            delete[] columnInfoList->columnInfo[i].name;
        }
        delete[] columnInfoList->columnInfo;
    }
}

void ContainerInfo::setExpiration(const Napi::CallbackInfo &info,
        const Napi::Value &value) {
    ExpirationInfo* expiration = Napi::ObjectWrap<ExpirationInfo>::Unwrap(
            value.As<Napi::Object>());

    if (mContainerInfo.timeSeriesProperties != NULL) {
        delete mContainerInfo.timeSeriesProperties;
        mContainerInfo.timeSeriesProperties = NULL;
    }
    if (expiration) {
        GSTimeSeriesProperties* ts;
        try {
            ts = new GSTimeSeriesProperties();
        } catch (std::bad_alloc& ba) {
            THROW_EXCEPTION_WITH_STR(
                    info.Env(), "Memory allocation error", NULL)
            return;
        }

        memcpy(ts, expiration->gs_ts(), sizeof(GSTimeSeriesProperties));

        mContainerInfo.timeSeriesProperties = ts;
    }
}

Napi::Value ContainerInfo::getExpiration(
        const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    int time;
    GSTimeUnit unit;
    int division_count;
    if (mContainerInfo.timeSeriesProperties != NULL) {
        time = mContainerInfo.timeSeriesProperties->rowExpirationTime;
        unit = mContainerInfo.timeSeriesProperties->rowExpirationTimeUnit;
        division_count =
                mContainerInfo.timeSeriesProperties->expirationDivisionCount;
        // Create new ExpirationInfo object
        Napi::EscapableHandleScope scope(env);
        return scope.Escape(ExpirationInfo::constructor.New( {
                Napi::Number::New(env, time), Napi::Number::New(env, unit),
                Napi::Number::New(env, division_count) })).ToObject();
    } else {
        return env.Null();
    }
}

/**
 * @brief Get all information of Container
 * @return A pointer which store all information of Container
 */
GSContainerInfo* ContainerInfo::gs_info() {
    return &mContainerInfo;
}

}  // namespace griddb

