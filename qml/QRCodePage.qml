import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Page {
    property alias uri: generator.text

    forwardNavigation: false
    backNavigation: false

    HarbourQrCodeGenerator {
        id: generator

        ecLevel: FoilAuthSettings.qrCodeEcLevel
    }

    QRCodeImage {
        qrcode: generator.qrcode
        anchors.centerIn: parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: pageStack.pop()
        onPressAndHold: /* ignore */;
    }
}
