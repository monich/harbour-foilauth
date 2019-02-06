import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    allowedOrientations: window.allowedOrientations

    DialogHeader {
        id: header

        //: Dialog button (accept the warning and continue)
        //% "Continue"
        acceptText: qsTrId("foilauth-generate_key_warning-accept")
    }

    Column {
        id: column

        spacing: Theme.paddingLarge
        anchors{
            top: header.bottom
            topMargin: Theme.paddingLarge
            left: parent.left
            right: parent.right
        }

        InfoLabel {
            //: Title for the new key warning
            //% "Warning"
            text: qsTrId("foilauth-generate_key_warning-title")
            font.bold: true
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            //: Warning shown prior to generating the new key
            //% "Once you have generated a new key, you are going to lose access to all the files encrypted by the old key. Note that the same key is used by all Foil apps, such as Foil Notes and Foil Pics. If you have forgotten your password, then keep in mind that most likely it's computationally easier to brute-force your password and recover the old key than to decrypt files for which the key is lost."
            text: qsTrId("foilauth-generate_key_warning-text")
            wrapMode: Text.Wrap
            color: Theme.highlightColor
        }
    }
}
