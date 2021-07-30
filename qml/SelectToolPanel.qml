import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Item {
    id: thisPanel

    height: Math.max(deleteButton.height, exportButton.height) + 2 * Theme.paddingMedium
    y: parent.height - visiblePart
    visible: visiblePart > 0

    property bool active
    property bool canDelete: active
    property bool canExport: active
    property real visiblePart: active ? height : 0

    signal exportSelectedItems()
    signal deleteSelectedItems()
    signal showDeleteHint()
    signal showExportHint()
    signal hideHint()

    Behavior on visiblePart { SmoothedAnimation { duration: 250  } }

    anchors {
        left: parent.left
        right: parent.right
    }

    HarbourIconTextButton {
        id: deleteButton

        x: Math.floor(thisPanel.width/4 - width/2)
        anchors.verticalCenter: parent.verticalCenter
        iconSource: "image://theme/icon-m-delete"
        enabled: canDelete
        onClicked: thisPanel.deleteSelectedItems()
        onPressAndHold: thisPanel.showDeleteHint()
        onPressedChanged: if (!pressed) thisPanel.hideHint()
    }

    HarbourIconTextButton {
        id: exportButton

        x: Math.floor(thisPanel.width*3/4 - width/2)
        anchors.verticalCenter: parent.verticalCenter
        iconSource: Qt.resolvedUrl("images/qr-export.svg")
        enabled: canExport
        onClicked: thisPanel.exportSelectedItems()
        onPressAndHold: thisPanel.showExportHint()
        onPressedChanged: if (!pressed) thisPanel.hideHint()
    }
}
