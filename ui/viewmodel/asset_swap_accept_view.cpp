// Copyright 2022 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License"){}
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

#include "asset_swap_accept_view.h"

#include "model/app_model.h"
#include "viewmodel/qml_globals.h"
#include "wallet/transactions/dex/dex_tx.h"

namespace
{
    const char kNoValue[] = "-";
}

AssetSwapAcceptViewModel::AssetSwapAcceptViewModel()
    : _walletModel(AppModel::getInstance().getWalletModel())
    , _amgr(AppModel::getInstance().getAssets())
{
    connect(_walletModel.get(), &WalletModel::dexOrdersFinded, this, &AssetSwapAcceptViewModel::onDexOrdersFinded);
}

void AssetSwapAcceptViewModel::startSwap()
{
    auto params = beam::wallet::CreateDexTransactionParams(
                    _orderId,
                    _sbbsID,
                    _sendAsset,
                    _amountToSendGrothes,
                    _receiveAsset,
                    _amountToReceiveGrothes);

    _walletModel->getAsync()->startTransaction(std::move(params));
}

void AssetSwapAcceptViewModel::onDexOrdersFinded(const beam::wallet::DexOrder& order)
{
    _sbbsID = order.getSBBSID();
    _amountToReceiveGrothes = order.getReceiveAmount();
    _amountToSendGrothes = order.getSendAmount();

    _receiveAsset = order.getReceiveAssetId();
    _receiveAssetSname = order.getReceiveAssetSName();

    _sendAsset = order.getSendAssetId();
    _sendAssetSname = order.getSendAssetSName();

    _offerCreated = order.getCreation();
    _offerExpires = order.getExpiration();
    emit orderChanged();
}

QString AssetSwapAcceptViewModel::getAmountToReceive() const
{
    if (!_amountToReceiveGrothes)
        return kNoValue;
    return beamui::AmountToUIString(_amountToReceiveGrothes);
}

QString AssetSwapAcceptViewModel::getAmountToSend() const
{
    if (!_amountToSendGrothes)
        return kNoValue;
    return beamui::AmountToUIString(_amountToSendGrothes);
}

QString AssetSwapAcceptViewModel::getFee() const
{
    return "-";
}

QString AssetSwapAcceptViewModel::getOfferCreated() const
{
    if (!_offerCreated)
        return kNoValue;

    QDateTime datetime;
    datetime.setTime_t(_offerCreated);
    return datetime.toString(_locale.dateTimeFormat(QLocale::ShortFormat));
}

QString AssetSwapAcceptViewModel::getOfferExpires() const
{
    if (!_offerExpires)
        return kNoValue;

    QDateTime datetime;
    datetime.setTime_t(_offerExpires);
    return datetime.toString(_locale.dateTimeFormat(QLocale::ShortFormat));
}

QString AssetSwapAcceptViewModel::getComment() const
{
    return _comment;
}

void AssetSwapAcceptViewModel::setComment(QString value)
{
    _comment = value;
    emit commentChanged();
}

QString AssetSwapAcceptViewModel::getRate() const
{
    if (!_amountToReceiveGrothes || !_amountToSendGrothes)
        return kNoValue;

    return QMLGlobals::divideWithPrecision(
                beamui::AmountToUIString(_amountToReceiveGrothes),
                beamui::AmountToUIString(_amountToSendGrothes),
                beam::wallet::kDexOrderRatePrecission);
}

QString AssetSwapAcceptViewModel::getOrderId() const
{
    return QString::fromStdString(_orderId.to_string());
}

void AssetSwapAcceptViewModel::setOrderId(QString value)
{
    beam::wallet::DexOrderID dexOrderId;
    if (dexOrderId.FromHex(value.toStdString()))
    {
        _orderId = dexOrderId;
        _walletModel->getAsync()->getDexOrder(dexOrderId);
    }
    else
    {
        _errorStr = "DexView::setOrderId bad order id";
    }
}

QList<QMap<QString, QVariant>> AssetSwapAcceptViewModel::getSendCurrencies() const
{
    return getCurrenciesList(_sendAsset, _sendAssetSname);
}

QList<QMap<QString, QVariant>> AssetSwapAcceptViewModel::getReceiveCurrencies() const
{
    return getCurrenciesList(_receiveAsset, _receiveAssetSname);
}

QList<QMap<QString, QVariant>> AssetSwapAcceptViewModel::getCurrenciesList(
    beam::Asset::ID assetId, const std::string& assetSname) const
{
    QList<QMap<QString, QVariant>> result;
    QMap<QString, QVariant> info;

    info.insert("isBEAM",     assetId == beam::Asset::s_BeamID);
    info.insert("unitName",   QString::fromStdString(assetSname));
    info.insert("icon",       _amgr->getIcon(assetId));
    info.insert("iconWidth",  22);
    info.insert("iconHeight", 22);
    info.insert("rate",       "-");
    info.insert("rateUnit",   "-");

    result.push_back(info);

    return result;
}