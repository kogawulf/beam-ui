// Copyright 2018 The Beam Team
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
#include "send_swap_view.h"
#include "model/app_model.h"
#include "qml_globals.h"
#include "wallet/transactions/swaps/common.h"
#include "wallet/transactions/swaps/swap_transaction.h"
#include "ui_helpers.h"
#include "fee_helpers.h"
#include "atomic_swap/swap_utils.h"
#include "wallet/transactions/swaps/bridges/ethereum/ethereum_side.h"
#include <algorithm>
#include <regex>

SendSwapViewModel::SendSwapViewModel()
    : _sendAmountGrothes(0)
    , _sendFeeGrothes(0)
    , _sendCurrency(OldWalletCurrency::OldCurrency::CurrStart)
    , _receiveAmountGrothes(0)
    , _receiveFeeGrothes(0)
    , _receiveCurrency(OldWalletCurrency::OldCurrency::CurrStart)
    , _changeGrothes(0)
    , _walletModel(*AppModel::getInstance().getWalletModel())
    , _isBeamSide(true)
    , _minimalBeamFeeGrothes(minimalFee(OldWalletCurrency::OldCurrency::CurrBeam, false))
{
    connect(&_walletModel, &WalletModel::changeCalculated,  this,  &SendSwapViewModel::onChangeCalculated);
    connect(&_walletModel, &WalletModel::walletStatusChanged, this, &SendSwapViewModel::recalcAvailable);
    connect(&_exchangeRatesManager, SIGNAL(rateUnitChanged()), SIGNAL(secondCurrencyUnitNameChanged()));
    connect(&_exchangeRatesManager, SIGNAL(activeRateChanged()), SIGNAL(secondCurrencyRateChanged()));
    connect(&_walletModel, &WalletModel::coinsSelected, this, &SendSwapViewModel::onCoinsSelected);
}

QString SendSwapViewModel::getToken() const
{
    return _token;
}

void SendSwapViewModel::fillParameters(const beam::wallet::TxParameters& parameters)
{
    // Set currency before fee, otherwise it would be reset to default fee
    using namespace beam::wallet;
    using namespace beam;

    auto isBeamSide = parameters.GetParameter<bool>(TxParameterID::AtomicSwapIsBeamSide);
    auto swapCoin = parameters.GetParameter<AtomicSwapCoin>(TxParameterID::AtomicSwapCoin);
    auto beamAmount = parameters.GetParameter<Amount>(TxParameterID::Amount);
    auto swapAmount = parameters.GetParameter<Amount>(TxParameterID::AtomicSwapAmount);
    auto peerID = parameters.GetParameter<WalletID>(TxParameterID::PeerID);
    auto peerResponseTime = parameters.GetParameter<Height>(TxParameterID::PeerResponseTime);
    auto offeredTime = parameters.GetParameter<Timestamp>(TxParameterID::CreateTime);
    auto minHeight = parameters.GetParameter<Height>(TxParameterID::MinHeight);

    if (peerID && swapAmount && beamAmount && swapCoin && isBeamSide
        && peerResponseTime && offeredTime && minHeight)
    {
        if (*isBeamSide) // other participant is not a beam side
        {
            // Do not set fee, it is set automatically based on the currency param
            setSendCurrency(OldWalletCurrency::OldCurrency::CurrBeam);
            setSendAmount(beamui::AmountToUIString(*beamAmount));
            setReceiveCurrency(convertSwapCoinToCurrency(*swapCoin));
            setReceiveAmount(beamui::AmountToUIString(*swapAmount, beamui::convertSwapCoinToCurrency(*swapCoin), false));
        }
        else
        {
            // Do not set fee, it is set automatically based on the currency param
            setSendCurrency(convertSwapCoinToCurrency(*swapCoin));
            setSendAmount(beamui::AmountToUIString(*swapAmount, beamui::convertSwapCoinToCurrency(*swapCoin), false));
            setReceiveCurrency(OldWalletCurrency::OldCurrency::CurrBeam);
            setReceiveAmount(beamui::AmountToUIString(*beamAmount));
        }
        setOfferedTime(QDateTime::fromSecsSinceEpoch(*offeredTime));

        auto currentHeight = _walletModel.getCurrentHeight();
        assert(currentHeight);
        beam::Timestamp currentHeightTime = _walletModel.getCurrentHeightTimestamp();
        auto expiresHeight = *minHeight + *peerResponseTime;
        setExpiresTime(beamui::CalculateExpiresTime(currentHeightTime, currentHeight, expiresHeight));

        _txParameters = parameters;
        _isBeamSide = *isBeamSide;
    }

    _tokenGeneratebByNewAppVersionMessage.clear();

    ProcessLibraryVersion(parameters, [this](const auto& version, const auto& myVersion)
    {
/*% "This address generated by newer Beam library version(%1)
Your version is: %2. Please, check for updates."
*/
        _tokenGeneratebByNewAppVersionMessage = qtTrId("swap-token-newer-lib")
            .arg(version.c_str())
            .arg(myVersion.c_str());
        emit tokenGeneratebByNewAppVersion();
    });

#ifdef BEAM_CLIENT_VERSION
    ProcessClientVersion(parameters, AppModel::getMyName(), BEAM_CLIENT_VERSION, [this](const auto& version, const auto& myVersion)
    {
/*% "This address generated by newer Beam client version(%1)
Your version is: %2. Please, check for updates."
*/
        _tokenGeneratebByNewAppVersionMessage = qtTrId("swap-swap-token-newer-client")
            .arg(version.c_str())
            .arg(myVersion.c_str());
        emit tokenGeneratebByNewAppVersion();
    });
#endif // BEAM_CLIENT_VERSION
}

void SendSwapViewModel::setParameters(const QVariant& parameters)
{
    if (!parameters.isNull() && parameters.isValid())
    {
        auto p = parameters.value<beam::wallet::TxParameters>();
        fillParameters(p);
    }
}

void SendSwapViewModel::setToken(const QString& value)
{
    if (_token != value)
    {
        _token = value;
        auto parameters = beam::wallet::ParseParameters(_token.toStdString());
        if (getTokenValid() && parameters)
        {
            fillParameters(*parameters);
        }
        emit tokenChanged();
    }
}

bool SendSwapViewModel::getTokenValid() const
{
    return QMLGlobals::isSwapToken(_token);
}

bool SendSwapViewModel::getParametersValid() const
{
    auto type = _txParameters.GetParameter<beam::wallet::TxType>(beam::wallet::TxParameterID::TransactionType);
    return type && *type == beam::wallet::TxType::AtomicSwap;
}

QString SendSwapViewModel::getSendAmount() const
{
    return beamui::AmountToUIString(_sendAmountGrothes, convertCurrency(_sendCurrency), false);
}

void SendSwapViewModel::setSendAmount(QString value)
{
    const auto amount = beamui::UIStringToAmount(value, convertCurrency(_sendCurrency));
    if (amount != _sendAmountGrothes)
    {
        _sendAmountGrothes = amount;
        emit sendAmountChanged();
        emit isSendFeeOKChanged();
        recalcAvailable();

        if (_sendCurrency == OldWalletCurrency::OldCurrency::CurrBeam && _walletModel.hasShielded(beam::Asset::s_BeamID))
        {
            _walletModel.getAsync()->selectCoins(_sendAmountGrothes, _sendFeeGrothes, beam::Asset::s_BeamID);
        }
    }
}

unsigned int SendSwapViewModel::getSendFee() const
{
    return _sendFeeGrothes;
}

void SendSwapViewModel::setSendFee(unsigned int value)
{
    if (value != _sendFeeGrothes)
    {
        _sendFeeGrothes = value;
        emit sendFeeChanged();
        emit isSendFeeOKChanged();
        recalcAvailable();

        if (_sendCurrency == OldWalletCurrency::OldCurrency::CurrBeam && _walletModel.hasShielded(beam::Asset::s_BeamID) && _sendAmountGrothes)
        {
            _feeChangedByUI = true;
            _walletModel.getAsync()->selectCoins(_sendAmountGrothes, _sendFeeGrothes, beam::Asset::s_BeamID);
        }
    }
}

OldWalletCurrency::OldCurrency SendSwapViewModel::getSendCurrency() const
{
    return _sendCurrency;
}

void SendSwapViewModel::setSendCurrency(OldWalletCurrency::OldCurrency value)
{
    assert(value > OldWalletCurrency::OldCurrency::CurrStart && value < OldWalletCurrency::OldCurrency::CurrEnd);

    if (value != _sendCurrency)
    {
        _sendCurrency = value;
        emit sendCurrencyChanged();
        emit isSendFeeOKChanged();
        recalcAvailable();
    }
}

QString SendSwapViewModel::getReceiveAmount() const
{
    return beamui::AmountToUIString(_receiveAmountGrothes, convertCurrency(_receiveCurrency), false);
}

void SendSwapViewModel::setReceiveAmount(QString value)
{
    const auto amount = beamui::UIStringToAmount(value, convertCurrency(_receiveCurrency));
    if (amount != _receiveAmountGrothes)
    {
        _receiveAmountGrothes = amount;
        emit receiveAmountChanged();
        emit isReceiveFeeOKChanged();
    }
}

unsigned int SendSwapViewModel::getReceiveFee() const
{
    return _receiveFeeGrothes;
}

void SendSwapViewModel::setReceiveFee(unsigned int value)
{
    if (value != _receiveFeeGrothes)
    {
        _receiveFeeGrothes = value;
        emit receiveFeeChanged();
        emit canSendChanged();
        emit isReceiveFeeOKChanged();
        emit enoughToReceiveChanged();
    }
}

OldWalletCurrency::OldCurrency SendSwapViewModel::getReceiveCurrency() const
{
    return _receiveCurrency;
}

void SendSwapViewModel::setReceiveCurrency(OldWalletCurrency::OldCurrency value)
{
    assert(value > OldWalletCurrency::OldCurrency::CurrStart && value < OldWalletCurrency::OldCurrency::CurrEnd);

    if (value != _receiveCurrency)
    {
        _receiveCurrency = value;
        emit receiveCurrencyChanged();
        emit isReceiveFeeOKChanged();
    }
}

QString SendSwapViewModel::getComment() const
{
    return _comment;
}

void SendSwapViewModel::setComment(const QString& value)
{
    if (_comment != value)
    {
        _comment = value;
        emit commentChanged();
    }
}

QDateTime SendSwapViewModel::getOfferedTime() const
{
    return _offeredTime;
}

void SendSwapViewModel::setOfferedTime(const QDateTime& value)
{
    if (_offeredTime != value)
    {
        _offeredTime = value;
        emit offeredTimeChanged();
    }
}

QDateTime SendSwapViewModel::getExpiresTime() const
{
    return _expiresTime;
}

void SendSwapViewModel::setExpiresTime(const QDateTime& value)
{
    if (_expiresTime != value)
    {
        _expiresTime = value;
        emit expiresTimeChanged();
    }
}

void SendSwapViewModel::onChangeCalculated(beam::Amount changeAsset, beam::Amount changeBeam, beam::Asset::ID assetID)
{
    using namespace beam;

    // only BEAM used in swap for the moment
    assert(assetID == Asset::s_BeamID);
    assert(AmountBig::get_Hi(changeAsset) == 0);
    assert(changeBeam == AmountBig::get_Lo(changeAsset));

    _changeGrothes = changeBeam;
    emit enoughChanged();
    emit canSendChanged();
}

void SendSwapViewModel::onCoinsSelected(const beam::wallet::CoinsSelectionInfo& selectionRes)
{
    if (_sendCurrency == OldWalletCurrency::OldCurrency::CurrBeam)
    {
        _minimalBeamFeeGrothes = selectionRes.m_minimalExplicitFee;
        emit minimalBeamFeeGrothesChanged();

        if (_feeChangedByUI)
        {
            _feeChangedByUI = false;
            return;
        }
        _sendFeeGrothes = selectionRes.m_explicitFee;
        emit sendFeeChanged();
    }
}

bool SendSwapViewModel::isEnough() const
{
    auto total = _sendAmountGrothes + _sendFeeGrothes + _changeGrothes;
    if (OldWalletCurrency::OldCurrency::CurrBeam == _sendCurrency)
    {
        auto available = beam::AmountBig::get_Lo(_walletModel.getAvailable(beam::Asset::s_BeamID));
        return available >= total;
    }

    auto swapCoin = convertCurrencyToSwapCoin(_sendCurrency);
    if (isEthereumBased(_sendCurrency))
    {
        if (_sendCurrency == OldWalletCurrency::OldCurrency::CurrEthereum)
        {
            total = _sendAmountGrothes + beam::wallet::EthereumSide::CalcLockTxFee(_sendFeeGrothes, swapCoin);

            return AppModel::getInstance().getSwapEthClient()->getAvailable(swapCoin) >= total;
        }

        return AppModel::getInstance().getSwapEthClient()->getAvailable(swapCoin) >= _sendAmountGrothes &&
            AppModel::getInstance().getSwapEthClient()->getAvailable(beam::wallet::AtomicSwapCoin::Ethereum) >=
            beam::wallet::EthereumSide::CalcLockTxFee(_sendFeeGrothes, swapCoin);
    }

    // TODO sentFee is fee rate. should be corrected
    return AppModel::getInstance().getSwapCoinClient(swapCoin)->getAvailable() > total;
}

bool SendSwapViewModel::isEnoughToReceive() const
{
    if (isEthereumBased(_receiveCurrency))
    {
        auto swapCoin = convertCurrencyToSwapCoin(_receiveCurrency);
        auto fee = beam::wallet::EthereumSide::CalcWithdrawTxFee(_receiveFeeGrothes, swapCoin);

        return AppModel::getInstance().getSwapEthClient()->getAvailable(beam::wallet::AtomicSwapCoin::Ethereum) > fee;
    }
    return true;
}

void SendSwapViewModel::recalcAvailable()
{
    switch(_sendCurrency)
    {
    case OldWalletCurrency::OldCurrency::CurrBeam:
        _changeGrothes = 0;
        _walletModel.getAsync()->calcChange(_sendAmountGrothes, _sendFeeGrothes, beam::Asset::s_BeamID);
        return;
    default:
        // TODO:SWAP implement for all currencies
        _changeGrothes = 0;
    }

    emit enoughChanged();
    emit canSendChanged();
}

QString SendSwapViewModel::getReceiverAddress() const
{
    auto peerID = _txParameters.GetParameter<beam::wallet::WalletID>(beam::wallet::TxParameterID::PeerID);
    if (peerID)
    {
        return beamui::toString(*peerID);
    }
    return _token;
}

bool SendSwapViewModel::canSend() const
{
    // TODO:SWAP check if correct
    return isFeeOK(_sendFeeGrothes, _sendCurrency, false) &&
           _sendCurrency != _receiveCurrency &&
           isEnough() &&
           QDateTime::currentDateTime() < _expiresTime;
}

void SendSwapViewModel::sendMoney()
{
    using beam::wallet::TxParameterID;
    
    auto txParameters = beam::wallet::TxParameters(_txParameters);
    auto beamFee = _isBeamSide ? getSendFee() : getReceiveFee();
    auto swapFee = _isBeamSide ? getReceiveFee() : getSendFee();

    beam::wallet::FillSwapFee(
        &txParameters,
        beam::Amount(beamFee),
        beam::Amount(swapFee),
        _isBeamSide);

    if (!_comment.isEmpty())
    {
        std::string localComment = _comment.toStdString();
        txParameters.SetParameter(TxParameterID::Message, beam::ByteBuffer(localComment.begin(), localComment.end()));
    }

    {
        auto txID = txParameters.GetTxID();
        auto swapCoin = txParameters.GetParameter<beam::wallet::AtomicSwapCoin>(TxParameterID::AtomicSwapCoin);
        auto amount = txParameters.GetParameter<beam::Amount>(TxParameterID::Amount);
        auto swapAmount = txParameters.GetParameter<beam::Amount>(TxParameterID::AtomicSwapAmount);
        auto responseHeight = txParameters.GetParameter<beam::Height>(TxParameterID::PeerResponseTime);
        auto minimalHeight = txParameters.GetParameter<beam::Height>(TxParameterID::MinHeight);

        LOG_INFO() << *txID << " Accept offer.\n\t"
                    << "isBeamSide: " << (_isBeamSide ? "true" : "false") << "\n\t"
                    << "swapCoin: " << std::to_string(*swapCoin) << "\n\t"
                    << "amount: " << *amount << "\n\t"
                    << "swapAmount: " << *swapAmount << "\n\t"
                    << "responseHeight: " << *responseHeight << "\n\t"
                    << "minimalHeight: " << *minimalHeight;
    }

    _walletModel.getAsync()->startTransaction(std::move(txParameters));
}

bool SendSwapViewModel::isSendFeeOK() const
{
    return _sendAmountGrothes == 0 || isSwapFeeOK(_sendAmountGrothes, _sendFeeGrothes, _sendCurrency);
}

bool SendSwapViewModel::isReceiveFeeOK() const
{
    return _receiveAmountGrothes == 0 || isSwapFeeOK(_receiveAmountGrothes, _receiveFeeGrothes, _receiveCurrency);
}

bool SendSwapViewModel::isSendBeam() const
{
    return _isBeamSide;
}

QString SendSwapViewModel::getRate() const
{
    beam::Amount otherCoinAmount =
        isSendBeam() ? _receiveAmountGrothes : _sendAmountGrothes;
    beam::Amount beamAmount =
        isSendBeam() ? _sendAmountGrothes : _receiveAmountGrothes;

    if (!beamAmount) return QString();

    beamui::Currencies otherCurrency =
        convertCurrency(isSendBeam() ? _receiveCurrency : _sendCurrency);

    return QMLGlobals::divideWithPrecision(
        beamui::AmountToUIString(otherCoinAmount, otherCurrency, false),
        beamui::AmountToUIString(beamAmount),
        beamui::getCurrencyDecimals(otherCurrency));
}

QString SendSwapViewModel::getSecondCurrencySendRateValue() const
{
    auto sendCurrency = ExchangeRatesManager::convertCurrencyToExchangeCurrency(getSendCurrency());
    auto rate = _exchangeRatesManager.getRate(sendCurrency);
    return beamui::AmountToUIString(rate);
}

QString SendSwapViewModel::getSecondCurrencyReceiveRateValue() const
{
    auto receiveCurrency = ExchangeRatesManager::convertCurrencyToExchangeCurrency(getReceiveCurrency());
    auto rate = _exchangeRatesManager.getRate(receiveCurrency);
    return beamui::AmountToUIString(rate);
}

QString SendSwapViewModel::getSecondCurrencyUnitName() const
{
    return beamui::getCurrencyUnitName(_exchangeRatesManager.getRateCurrency());
}

bool SendSwapViewModel::isTokenGeneratedByNewVersion() const
{
    return !_tokenGeneratebByNewAppVersionMessage.isEmpty();
}

QString SendSwapViewModel::tokenGeneratedByNewVersionMessage() const
{
    return _tokenGeneratebByNewAppVersionMessage;
}

unsigned int SendSwapViewModel::getMinimalBeamFeeGrothes() const
{
    return _minimalBeamFeeGrothes;
}

QString SendSwapViewModel::getSentFeeTitle() const
{
    return swapui::getSwapFeeTitle(_sendCurrency);
}

QString SendSwapViewModel::getReceiveFeeTitle() const
{
    return swapui::getSwapFeeTitle(_receiveCurrency);
}

QList<QMap<QString, QVariant>> SendSwapViewModel::getCurrList() const
{
    return swapui::getUICurrList();
}
