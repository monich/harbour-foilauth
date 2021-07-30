import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Dialog {
    id: thisPage

    property alias uri: generator.text
    property Page mainPage
    property var exportList
    property int currentIndex

    readonly property bool haveNext: (currentIndex + 1) < exportList.length

    acceptDestination: haveNext ? Qt.resolvedUrl("QRCodeExportPage.qml") : mainPage
    acceptDestinationAction: haveNext ? PageStackAction.Push : PageStackAction.Pop
    acceptDestinationProperties: haveNext ? {
        "allowedOrientations": allowedOrientations,
        "mainPage": mainPage,
        "exportList": exportList,
        "currentIndex": currentIndex + 1
    } : undefined

    HarbourQrCodeGenerator {
        id: generator

        text: exportList[currentIndex]
        ecLevel: FoilAuthSettings.qrCodeEcLevel
    }

    Label {
        id: header

        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        height: isLandscape ? Theme.itemSizeSmall : Theme.itemSizeLarge
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: Theme.highlightColor
        font {
            pixelSize: Theme.fontSizeLarge
            family: Theme.fontFamilyHeading
            bold: true
        }
        //: Page header
        //% "Code %1 of %2"
        text: qsTrId("foilauth-export_page-title").arg(currentIndex + 1).arg(exportList.length)
    }

    Item {
        width: parent.width
        anchors {
            top: header.bottom
            bottom: parent.bottom
        }

        QRCodeImage {
            maximumSize: Math.min(parent.width, parent.height) - 2 * Theme.horizontalPageMargin
            qrcode: generator.qrcode
            anchors.centerIn: parent
        }
    }
}
