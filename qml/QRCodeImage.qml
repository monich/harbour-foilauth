import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    property string qrcode

    color: "white"
    radius: Theme.paddingMedium
    width: qrcodeImage.width + 2 * Theme.horizontalPageMargin
    height: qrcodeImage.height + 2 * Theme.horizontalPageMargin
    visible: qrcode !== ""

    Image {
        id: qrcodeImage

        asynchronous: true
        anchors.centerIn: parent
        source: qrcode ? "image://qrcode/" + qrcode : ""
        readonly property int maxDisplaySize: Math.min(Screen.width, Screen.height) - Theme.itemSizeLarge - 4 * Theme.horizontalPageMargin
        readonly property int maxSourceSize: Math.max(sourceSize.width, sourceSize.height)
        readonly property int n: Math.floor(maxDisplaySize/maxSourceSize)
        width: sourceSize.width * n
        height: sourceSize.height * n
        smooth: false
    }
}
