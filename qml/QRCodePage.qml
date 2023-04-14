import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Page {
    property alias uri: generator.text
    property alias label: header.title
    property alias issuer: header.description

    forwardNavigation: false

    PageHeader { id: header }

    Item {
        anchors {
            left: parent.left
            right: parent.right
            top: header.bottom
            bottom: parent.bottom
        }

        QRCodeImage {
            qrcode: generator.qrcode
            anchors.centerIn: parent
            maximumSize: Math.min(parent.width - 2 * Theme.horizontalPageMargin, parent.height - 2 * Theme.paddingLarge)
        }
    }

    HarbourQrCodeGenerator {
        id: generator

        ecLevel: FoilAuthSettings.qrCodeEcLevel
    }
}
