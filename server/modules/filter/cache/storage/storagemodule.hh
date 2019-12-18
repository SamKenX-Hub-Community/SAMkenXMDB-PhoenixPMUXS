/*
 * Copyright (c) 2018 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2023-12-18
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
#pragma once

#include <maxscale/ccdefs.hh>

template<class StorageType>
class StorageModuleT : public StorageModule
{
public:
    bool initialize(uint32_t* pCapabilities) override final
    {
        return StorageType::initialize(pCapabilities);
    }

    void finalize() override final
    {
        StorageType::finalize();
    }

    Storage* create_storage(const char* zName,
                            const Storage::Config& config,
                            int argc,
                            char* argv[]) override final
    {
        mxb_assert(zName);

        return StorageType::create(zName, config, argc, argv);
    }
};
