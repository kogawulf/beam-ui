import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import Beam.Wallet 1.0
import "."
import "../controls"
import "../utils.js" as Utils

Rectangle {
    color: Style.background_main
    property Item defaultFocusItem: null

    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.fill: parent
        anchors.topMargin: 50
        Column {
            spacing: 30
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.preferredWidth: 730
            SFText {
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Qt.AlignHCenter
                //% "Import Trezor Owner Key"
                text: qsTrId("start-import-trezor-owner-key")
                color: Style.content_main
                font.pixelSize: 36
            }
            SFText {
                anchors.left: parent.left
                anchors.right: parent.right
                horizontalAlignment: Qt.AlignHCenter
                text: viewModel.isOwnerKeyImported 
                    //% "Owner Key imported. Please, enter the password you saw on device to decrypt your Owner Key."
                    ? qsTrId("start-owner-key-imported")
                    //% "Please, look at your Trezor to complete actions..."
                    : qsTrId("start-look-at-trezor-to-complete-actions")
                color: Style.content_main
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }

            SFTextInput {
                id:trezorPassword
                width: 400
                anchors.horizontalCenter: parent.horizontalCenter
                visible: viewModel.isOwnerKeyImported
                font.pixelSize: 14
                color: Style.content_main
                echoMode: TextInput.Password
            }
        }


        Item {
            Layout.fillHeight: true
            Layout.minimumHeight: 120
        }

        Row {
            Layout.alignment: Qt.AlignHCenter

            spacing: 30

            CustomButton {
                //% "Back"
                text: qsTrId("general-back")
                enabled: viewModel.isOwnerKeyImported
                icon.source: "qrc:/assets/icon-back.svg"
                onClicked: startWizzardView.pop();
            }

            PrimaryButton {
                id: checkRecoveryNextButton
                //% "Next"
                text: qsTrId("general-next")
                enabled: viewModel.isOwnerKeyImported && viewModel.isPasswordValid(trezorPassword.text)
                icon.source: "qrc:/assets/icon-next-blue.svg"
                onClicked: {
                    viewModel.setOwnerKeyPassword(trezorPassword.text)
                    startWizzardView.push(accountLabelPage)
                }
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.minimumHeight: 67
            Layout.maximumHeight: 143
        }

        VersionFooter {}
    }
}