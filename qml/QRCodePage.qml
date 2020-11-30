import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Page {
    id: thisPage

    property string uri
    property alias qrcode: qrcodeImage.qrcode
    property var generator

    forwardNavigation: false
    backNavigation: false

    onUriChanged: syncUri()
    Component.onCompleted: syncUri()

    function syncUri() {
        if (uri !== "") {
            if (generator) {
                generator.text = uri
            } else {
                generator = generatorComponent.createObject(thisPage, { text: uri })
                qrcode = generator.qrcode
            }
        }
    }

    Component {
        id: generatorComponent

        HarbourQrCodeGenerator {
            onQrcodeChanged: qrcodeImage.qrcode = qrcode
        }
    }

    QRCodeImage {
        id: qrcodeImage

        anchors.centerIn: parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: pageStack.pop()
        onPressAndHold: /* ignore */;
    }
}
