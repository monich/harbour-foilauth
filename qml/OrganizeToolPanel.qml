import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Rectangle {
    id: thisItem

    property bool active
    property bool highlightReorderButton
    property bool highlightGroupButtom

    signal reorderTokens()
    signal groupTokens()

    height: Math.max(reorderButton.height, groupButton.height) + 2 * Theme.paddingMedium
    color: Theme.rgba(Theme.highlightBackgroundColor, 0.1)
    anchors {
        left: parent.left
        right: parent.right
    }

    MouseArea {
        id: reorderButtonArea

        width: thisItem.width/2
        height: parent.height

        property bool down: pressed && containsMouse

        HarbourIconTextButton {
            id: reorderButton

            anchors.centerIn: parent
            highlighted: down || reorderButtonArea.down || highlightReorderButton
            iconSource: "image://theme/icon-m-transfer"
            onClicked: thisItem.reorderTokens()
        }

        onClicked: thisItem.reorderTokens()
    }

    MouseArea {
        id: groupButtonArea

        x: width
        width: thisItem.width/2
        height: parent.height

        property bool down: pressed && containsMouse

        HarbourIconTextButton {
            id: groupButton

            anchors.centerIn: parent
            highlighted: down || groupButtonArea.down || highlightGroupButtom
            iconSource: Qt.resolvedUrl("images/folder.svg")
            onClicked: thisItem.groupTokens()
        }

        onClicked: thisItem.groupTokens()
    }
}
