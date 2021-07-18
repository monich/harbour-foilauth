import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Item {
    id: thisPanel

    height: deleteButton.height + 2 * Theme.paddingMedium
    y: parent.height - visiblePart
    visible: visiblePart > 0

    property bool active
    property bool canDelete: active
    property real visiblePart: active ? height : 0

    signal deleteSelectedItems()
    signal showDeleteHint()
    signal hideHint()

    Behavior on visiblePart { SmoothedAnimation { duration: 250  } }

    anchors {
        left: parent.left
        right: parent.right
    }

    HarbourIconTextButton {
        id: deleteButton

        x: Math.floor(thisPanel.width/2 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        iconSource: "image://theme/icon-m-delete"
        enabled: canDelete
        onClicked: thisPanel.deleteSelectedItems()
        onPressAndHold: thisPanel.showDeleteHint()
        onPressedChanged: if (!pressed) thisPanel.hideHint()
    }
}
