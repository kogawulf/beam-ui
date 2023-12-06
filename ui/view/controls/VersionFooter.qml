import QtQuick 2.12
import Beam.Wallet 1.0
import QtQuick.Layouts 1.15
import "."

SFText {
    Layout.alignment:    Qt.AlignHCenter
    Layout.bottomMargin: 27
    font.pixelSize:      12
    color:               Qt.rgba(255, 255, 255, 0.3)
    text:                [qsTrId("settings-version"), BeamGlobals.version()].join(' ')
}