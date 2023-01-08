import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Rectangle {
    id: root
    property string groupName
    property bool selected
    property bool highlighted
    property bool editing
    property alias editText: editor.text
    color: "transparent"

    signal doneEditing()

    function startEditing() {
        editor.forceActiveFocus()
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.15) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    Label {
        visible: !editing
        anchors {
            left: parent.left
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        truncationMode: TruncationMode.Fade
        text: groupName
        color: selected ? Theme.highlightColor : (highlighted ? Theme.highlightColor : Theme.primaryColor)
    }

    TextField {
        id: editor
        y: Theme.paddingSmall
        visible: editing
        anchors {
            left: parent.left
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        textLeftMargin: 0
        textRightMargin: 0
        horizontalAlignment: Text.AlignLeft
        labelVisible: false
        onActiveFocusChanged: if (!activeFocus) root.doneEditing()
        EnterKey.onClicked: root.focus = true
        EnterKey.iconSource: "image://theme/icon-m-enter-close"
    }
}
