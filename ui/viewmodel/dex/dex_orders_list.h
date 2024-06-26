// Copyright 2022 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "model/assets_manager.h"
#include "viewmodel/helpers/list_model.h"
#include "wallet/client/extensions/dex_board/dex_order.h"
#include <QLocale>

class DexOrdersList : public ListModel<beam::wallet::DexOrder>
{
    Q_OBJECT
public:
    DexOrdersList();
    ~DexOrdersList() override = default;

    enum class Roles
    {
        RId = Qt::UserRole + 1,
        RIdSort,
        RSend,
        RSendSort,
        RReceive,
        RReceiveSort,
        RRate,
        RRateSort,
        RIsMine,
        RIsMineSort,
        RCreateTime,
        RCreateTimeSort,
        RExpireTime,
        RExpireTimeSort,
        RCoins,
        RCoinsSort,
        RHasAssetToSend,
        RHasAssetToSendSort,
        RAssetsFilter
    };

    Q_ENUM(Roles)

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

public slots:
    void assetsListChanged();

private:
    QLocale m_locale; // default
    AssetsManager::Ptr m_amgr;
    WalletModel::Ptr m_wallet;
};
