import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    property bool firstTime: true
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
                anchors.verticalCenter: parent.verticalCenter
                source: "images/sailotp.svg"
                sourceSize: Qt.size(parent.height, parent.height)
                smooth: true
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: "\u2192"
                font.pixelSize: Theme.fontSizeHuge
            }

            Image {
                anchors.verticalCenter: parent.verticalCenter
                source: "images/foilauth.svg"
                sourceSize: Qt.size(parent.height, parent.height)
                smooth: true
            }
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            text: firstTime ?
                //: Text for SailOTP import page (first import, one token)
                //% "FoilAuth has found 1 unencrypted SailOTP token on your device. Would you like to import and encrypt it?"
                ((count === 1) ? qsTrId("foilauth-import-first_one") :
                //: Text for SailOTP import page (first import, multiple tokens)
                //% "FoilAuth has found %0 unencrypted SailOTP token(s) on your device. Would you like to import and encrypt them?"
                qsTrId("foilauth-import-first_many", count).arg(count)) :
                //: Text for SailOTP import page (one new token is found)
                //% "FoilAuth has found 1 new unencrypted SailOTP token on your device. Would you like to import and encrypt it?"
                ((count === 1) ? qsTrId("foilauth-import-new_one") :
                //: Text for SailOTP import page (multiple new tokens were found)
                //% "FoilAuth has found %0 new unencrypted SailOTP token(s) on your device. Would you like to import and encrypt them?"
                qsTrId("foilauth-import-new_many", count).arg(count))
            wrapMode: Text.Wrap
            color: Theme.highlightColor
        }
    }
}
