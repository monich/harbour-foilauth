import QtQuick 2.0
import Sailfish.Silica 1.0

IconButton {
    property string hint

    signal showHint(var text)
    signal hideHint()

    onClicked: hideHint()
    onReleased: hideHint()
    onCanceled: hideHint()
    onPressAndHold: if (hint !== "") showHint(hint)
}
