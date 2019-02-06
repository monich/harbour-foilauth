import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    allowedOrientations: appAllowedOrientations

    property int count

    DialogHeader {
        id: header

        //: Dialog button
        //% "No"
        cancelText: qsTrId("foilauth-import-cancel")

        //: Dialog button
        //% "Yes"
        acceptText: qsTrId("foilauth-import-accept")
    }

    Column {
        anchors{
            top: header.bottom
            topMargin: Theme.paddingLarge
            left: parent.left
            right: parent.right
        }
        spacing: Theme.paddingLarge

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingLarge
            height: Theme.itemSizeLarge

            Image {
                source: "images/sailotp.png"
                sourceSize: Qt.size(parent.height, parent.height)
                smooth: true
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: "\u2192"
                font.pixelSize: Theme.fontSizeHuge
            }

            Image {
                source: "images/foilauth.svg"
                sourceSize: Qt.size(parent.height, parent.height)
                smooth: true
            }
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            //: Text for import page
            //% "FoilAuth has found %0 unencrypted SailOTP token(s) on your device. Would you like to import and encrypt them?"
            text: qsTrId("foilauth-import-text",count).arg(count)
            wrapMode: Text.Wrap
            color: Theme.highlightColor
        }
    }
}
