import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    property string qrcode
    property int maximumSize: Math.min(Screen.width, Screen.height) - 2 * Theme.horizontalPageMargin
    property int margins: Theme.horizontalPageMargin

    color: "white"
    radius: Theme.paddingMedium
    width: implicitWidth
    height: implicitHeight
    visible: qrcode !== ""
    implicitWidth: qrcodeImage.width + 2 * margins
    implicitHeight: qrcodeImage.height + 2 * margins

    Image {
        id: qrcodeImage

        asynchronous: true
        anchors.centerIn: parent
        source: qrcode ? "image://qrcode/" + qrcode : ""
        readonly property int minimumSize: Math.max(sourceSize.width, sourceSize.height)
        readonly property int n: Math.max(Math.floor((maximumSize - 2 * margins)/minimumSize), 1)
        width: sourceSize.width * n
        height: sourceSize.height * n
        smooth: false
    }
}
