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

#include <napi.h>
#include "GSException.h"
#include "StoreFactory.h"
#include "ContainerInfo.h"
#include "ExpirationInfo.h"
#include "AggregationResult.h"
#include "Container.h"
#include "PartitionController.h"
#include "Query.h"
#include "RowSet.h"
#include "Store.h"
#include "RowKeyPredicate.h"
#include "QueryAnalysisEntry.h"
#include "Util.h"

Napi::Object init(Napi::Env env, Napi::Object exports);

using namespace griddb;

Napi::Object init(Napi::Env env, Napi::Object exports) {
#if NAPI_VERSION > 5
    env.SetInstanceData<AddonData, Util::addonDataFinalizer>(new AddonData());
#endif
    GSException::init(env, exports);
    StoreFactory::init(env, exports);
    Store::init(env, exports);
    ContainerInfo::init(env, exports);
    ExpirationInfo::init(env, exports);
    AggregationResult::init(env, exports);
    Container::init(env, exports);
    PartitionController::init(env, exports);
    Query::init(env, exports);
    RowSet::init(env, exports);
    RowKeyPredicate::init(env, exports);
    QueryAnalysisEntry::init(env, exports);
    return exports;
}

NODE_API_MODULE(addon, init);
