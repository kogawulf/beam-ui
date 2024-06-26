import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15
import QtQuick.Layouts 1.15
import Beam.Wallet 1.0
import "controls"
import "./utils.js" as Utils

ColumnLayout {
    id: control
    spacing: 0
    ReceiveViewModel {
        id: viewModel

        onNewAddressFailed: function () {
            var popup = Qt.createComponent("popup_message.qml").createObject(control)
            //% "You cannot generate new address. Your wallet doesn't have a master key."
            popup.message = qsTrId("can-not-generate-new-address-message")
            popup.open()
        }
    }

    property var defaultFocusItem: null
    property var onClosed: function () {} // set by parent
    property bool isShieldedSupported: statusbarModel.isConnectionTrusted && statusbarModel.isOnline
    property bool hasHwError: statusbarModel.hwwError.length

    property alias token:     viewModel.token
    property alias assetId:   viewModel.assetId
    property alias assetIdx:  amountInput.currencyIdx
    property var   assetInfo: viewModel.assetsList[control.assetIdx]

    function syncIdx () {
        for (var idx = 0; idx < viewModel.assetsList.length; ++idx) {
            if (viewModel.assetsList[idx].assetId == assetId) {
                 if (assetIdx != idx) {
                    assetIdx = idx
                 }
            }
        }
    }

    onAssetIdChanged: function () {
        // TODO: C++ sometimes provides asset id, combobox exepects index, need to fix this at some point
        syncIdx()
    }

    Component.onCompleted: function () {
        // asset id might be passed by other parts of the UI as a parameter to the receive view
        syncIdx()
    }

    Component.onDestruction: function () {
        viewModel.saveAddress()
    }

    TopGradient {
        mainRoot: main
        topColor: Style.accent_incoming
    }

    TokenInfoDialog {
        id:       tokenInfoDialog
        token:    viewModel.token
        incoming: true
    }

    function isValid () {
        return viewModel.commentValid
    }

    function copyAndClose() {
        copyAndSaveAddressAndClose(viewModel.token)
    }

    function copySBBSAndClose() {
        copyAndSaveAddressAndClose(viewModel.sbbsAddress)
    }

    function copyAndSave() {
         copyAndSaveAddress(viewModel.token);
    }

    function copySBBSAndSave() {
        copyAndSaveAddress(viewModel.sbbsAddress);
    }

    function copyAndSaveAddress(address) {
        if (isValid()) {
           BeamGlobals.copyToClipboard(address);
           viewModel.saveAddress();
        }
    }

    function copyAndSaveAddressAndClose(address) {
        copyAndSaveAddress(address)
        control.onClosed()
    }

    //% "Receive"
    property string title: qsTrId("wallet-receive-title")

    QR {
        id: qrCode
        address: viewModel.token
    }

    ScrollView {
        id:                  scrollView
        Layout.fillWidth:    true
        Layout.fillHeight:   true
        Layout.bottomMargin: 10
        clip:                true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy:   ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth

            //
            // Content row
            //
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                //
                // Left column
                //
                ColumnLayout {
                    Layout.alignment:       Qt.AlignTop
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  400
                    spacing:                10

                    //
                    // Requested amount
                    //
                    FoldablePanel {
                        //% "Requested amount"
                        title:             qsTrId("receive-request")
                        //% "(optional)"
                        titleTip:          qsTrId("receive-request-optional")
                        Layout.fillWidth:  true
                        folded:            false

                        //
                        // Amount
                        //
                        content: AmountInput {
                            id:          amountInput
                            amount:      viewModel.amount
                            currencies:  viewModel.assetsList
                            multi:       viewModel.assetsList.length > 1
                            resetAmount: false

                           onCurrencyIdxChanged: function () {
                               var idx = amountInput.currencyIdx
                               control.assetId = viewModel.assetsList[idx].assetId
                           }
                        }
                
                        Binding {
                            target:   viewModel
                            property: "amount"
                            value:    amountInput.amount
                        }
                    }

                    //
                    // Comment
                    //
                    FoldablePanel {
                        //% "Comment"
                        title:            qsTrId("general-comment")
                        Layout.fillWidth: true
                        folded:           false

                        content: ColumnLayout {
                            spacing: 0

                            SFTextInput {
                                id:               commentInput
                                font.pixelSize:   14
                                Layout.fillWidth: true
                                font.italic :     !viewModel.commentValid
                                backgroundColor:  viewModel.commentValid ? Style.content_main : Style.validator_error
                                color:            viewModel.commentValid ? Style.content_main : Style.validator_error
                                focus:            true
                                text:             viewModel.comment
                                //% "Comments are local and won't be shared"
                                placeholderText:  qsTrId("general-comment-local")
                                maximumLength:    BeamGlobals.maxCommentLength()
                            }
                 
                            Binding {
                                target:   viewModel
                                property: "comment"
                                value:    commentInput.text
                            }
                 
                            Item {
                                Layout.fillWidth: true
                                SFText {
                                    //% "Address with the same comment already exists"
                                    text:           qsTrId("general-addr-comment-error")
                                    color:          Style.validator_error
                                    font.pixelSize: 12
                                    visible:        !viewModel.commentValid
                                    font.italic:    true
                                }
                            }
                        }
                    }

                    FoldablePanel {
                        //% "Advanced"
                        title: qsTrId("general-advanced")
                        Layout.fillWidth: true
                        folded: true

                        content: ColumnLayout {
                            spacing: 20
                            id: addressType
                            // property bool isShieldedSupported: statusbarModel.isConnectionTrusted && statusbarModel.isOnline

                            CustomSwitch {
                                id: txType
                                //% "Maximum anonymity set"
                                text: qsTrId("receive-max-set")
                                enabled: control.isShieldedSupported
                                palette.text: control.isShieldedSupported ? Style.content_main : Style.content_secondary
                                checked: viewModel.isMaxPrivacy

                                Binding {
                                    target: viewModel
                                    property: "isMaxPrivacy"
                                    value: txType.checked
                                }
                            }
                        }
                    }
                }

                //
                // Right column
                //
                ColumnLayout {
                    Layout.alignment:       Qt.AlignTop
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  400
                    spacing:                10

                    Panel {
                        //% "Address"
                        title: qsTrId("receive-addr")
                        Layout.fillWidth: true

                        content: ColumnLayout {
                            spacing: 12

                            RowLayout {
                                spacing:          0
                                Layout.alignment: Qt.AlignHCenter

                                Image {
                                    Layout.preferredWidth:  270
                                    Layout.preferredHeight: 270
                                    fillMode:               Image.PreserveAspectFit
                                    source:                 qrCode.data
                                }
                            }

                            ColumnLayout {
                                spacing: 0
                                visible: !control.hasHwError

                                RowLayout {
                                    spacing: 0

                                    Layout.fillWidth:   true
                                    Layout.leftMargin:  20
                                    Layout.rightMargin: 2
                                    Layout.alignment:   Qt.AlignVCenter

                                    SFText {
                                        Layout.fillWidth:   true
                                        text:  viewModel.token
                                        width: parent.width
                                        color: Style.content_main
                                        elide: Text.ElideMiddle
                                    }
                                    CustomToolButton {
                                        Layout.alignment: Qt.AlignVCenter
                                        icon.source: "qrc:/assets/icon-copy.svg"
                                        ToolTip.text: qsTrId("settings-swap-copy-address")
                                        ToolTip.visible: hovered
                                        ToolTip.delay: 500
                                        ToolTip.timeout: 2000
                                        enabled: control.isValid()
                                        onClicked: {
                                            control.copyAndSave()
                                        }
                                    }
                                    CustomToolButton {
                                        Layout.alignment: Qt.AlignVCenter
                                        icon.source: "qrc:/assets/icon-refresh.svg"
                                        //: receive screen, button to generate new address
                                        //% "Generate new address"
                                        ToolTip.text: qsTrId("receive-generate-new")
                                        ToolTip.visible: hovered
                                        ToolTip.delay: 500
                                        ToolTip.timeout: 2000
                                        hoverEnabled: true
                                        onClicked: {
                                            viewModel.generateNewAddress()
                                        }
                                    }
                                }

                                LinkButton {
                                    Layout.alignment: Qt.AlignHCenter
                                    //% "Address details"
                                    text:       qsTrId("address-details")
                                    linkColor:  Style.accent_incoming
                                    onClicked:  function () {
                                        tokenInfoDialog.open()
                                    }
                                }
                                Rectangle {
                                    Layout.fillWidth:        true
                                    Layout.preferredHeight:  childrenRect.height
                                    color:                   "transparent"
                                    border.color:            "orange"
                                    border.width:            2
                                    radius:                  10
                                    visible:                 viewModel.sbbsAddress.length > 0
                                    ColumnLayout {
                                        anchors.left:       parent.left
                                        anchors.right:      parent.right
                                        anchors.leftMargin: 20
                                        spacing:            0
                                        SFText {
                                            Layout.alignment:       Qt.AlignTop
                                            Layout.topMargin:       10
                                            font.pixelSize:         14
                                            color:                  Style.content_main
                                            text:                   qsTrId("address-info-sbbs-address") + ":"
                                        }
                                        RowLayout {
                                            spacing: 0

                                            Layout.fillWidth:   true
                                            Layout.rightMargin: 2
                                            Layout.alignment:   Qt.AlignVCenter

                                            SFText {
                                                id : sbbdText
                                                Layout.fillWidth:    true
                                                text:                viewModel.sbbsAddress
                                                width:               parent.width
                                                color:               Style.content_main
                                                elide:               Text.ElideMiddle
                                            }
                                            CustomToolButton {
                                                Layout.alignment: Qt.AlignVCenter
                                                icon.source: "qrc:/assets/icon-copy.svg"
                                                ToolTip.text: qsTrId("settings-swap-copy-address")
                                                ToolTip.visible: hovered
                                                ToolTip.delay: 500
                                                ToolTip.timeout: 2000
                                                enabled: control.isValid()
                                                onClicked: {
                                                    control.copySBBSAndSave()
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            SFText {
                                Layout.fillWidth:   true
                                width: parent.width
                                font.pixelSize:        14
                                font.italic:           true
                                color:                 Style.validator_error
                                wrapMode:              Text.WordWrap
                                horizontalAlignment:   Text.AlignHCenter
                                visible:               control.hasHwError
/*% "QR code is wrong.
A hardware wallet is not connected. Please, connect the wallet"
*/
                                text:                  qsTrId("receive-view-hw-wallet-disconnected")
                            }
                        }
                    }
                }
            }
            Row {
                Layout.topMargin:       30
                Layout.alignment:       Qt.AlignHCenter
                spacing:    20
                CustomButton {
                    id: copyButton
                    //% "copy and close"
                    text:                   qsTrId("general-copy-and-close")
                    Layout.preferredHeight: 38
                    palette.buttonText:     Style.content_opposite
                    icon.color:             Style.content_opposite
                    palette.button:         Style.accent_incoming
                    icon.source:            "qrc:/assets/icon-copy.svg"
                    enabled:                control.isValid()

                    onClicked: function () {
                        control.copyAndClose()
                    }
                }
                CustomButton {
                    id: copySbbsButton
                    //% "copy SBBS address and close"
                    text:                   qsTrId("general-copy-SBBS-and-close")
                    Layout.preferredHeight: 38
                    palette.buttonText:     Style.content_opposite
                    icon.color:             Style.content_opposite
                    palette.button:         Style.accent_outgoing
                    icon.source:            "qrc:/assets/icon-copy.svg"
                    enabled:                control.isValid()
                    visible:                viewModel.sbbsAddress.length > 0

                    onClicked: function () {
                        control.copySBBSAndClose()
                    }
                }
            }

            //
            // Footers
            //
            SFText {
                property int mpLockTimeLimit: viewModel.mpTimeLimit
                Layout.alignment:      Qt.AlignHCenter
                Layout.preferredWidth: 428
                Layout.topMargin:      25
                font.pixelSize:        14
                font.italic:           true
                color:                 Style.content_disabled
                wrapMode:              Text.WordWrap
                horizontalAlignment:   Text.AlignHCenter
                visible:               viewModel.isMaxPrivacy
                text: (mpLockTimeLimit ?
                    //% "Transaction can last at most %n hour(s)."
                    qsTrId("wallet-receive-addr-message-mp", mpLockTimeLimit) :
                    //% "Transaction can last indefinitely."
                    qsTrId("wallet-receive-addr-message-mp-no-limit"))
            }

            SFText {
                Layout.alignment:      Qt.AlignHCenter
                Layout.preferredWidth: 428
                font.pixelSize:        14
                font.italic:           true
                color:                 Style.content_disabled
                wrapMode:              Text.WordWrap
                horizontalAlignment:   Text.AlignHCenter
                visible:               viewModel.isMaxPrivacy
                //% "Min transaction fee is 0.01 BEAM."
                text: qsTrId("wallet-receive-addr-message-min-fee")
            }
        }  // ColumnLayout
    }  // ScrollView
}
