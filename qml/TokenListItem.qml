import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Rectangle {
    id: item

    color: "transparent"

    property alias description: descriptionLabel.text
    property string prevPassword
    property string nextPassword
    property string currentPassword
    property bool favorite
    property bool landscape

    signal favoriteToggled()

    IconButton {
        id: favoriteButton

        anchors {
            left: parent.left
            leftMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        icon.source: favorite ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
        onClicked: item.favoriteToggled()
    }

    Label {
        id: descriptionLabel

        anchors {
            left: favoriteButton.right
            leftMargin: landscape ? Theme.paddingLarge : 0
            rightMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
        truncationMode: TruncationMode.Fade
        horizontalAlignment: Text.AlignLeft
        color: Theme.highlightColor
        textFormat: Text.PlainText
    }

    Label {
        id: prevPasswordLabel

        anchors {
            right: currentPasswordLabel.left
            rightMargin: Theme.paddingLarge
            baseline: currentPasswordLabel.baseline
        }
        font.pixelSize: Theme.fontSizeTiny
        color: Theme.highlightColor
        visible: landscape
        transform: HarbourTextFlip {
            text: item.prevPassword
            target: prevPasswordLabel
        }
    }

    Label {
        id: currentPasswordLabel

        anchors {
            right: parent.right
            rightMargin: landscape ? Theme.paddingLarge : Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        font {
            pixelSize: Theme.fontSizeLarge
            family: Theme.fontFamilyHeading
            bold: true
        }
        transform: HarbourTextFlip {
            text: item.currentPassword
            target: currentPasswordLabel
        }
    }

    Label {
        id: nextPasswordLabel

        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            baseline: currentPasswordLabel.baseline
        }
        font.pixelSize: Theme.fontSizeTiny
        color: Theme.highlightColor
        visible: landscape
        transform: HarbourTextFlip {
            text: item.nextPassword
            target: nextPasswordLabel
        }
    }

    states: [
        State {
            name: "portrait"
            when: !landscape
            changes: [
                AnchorChanges {
                    target: descriptionLabel
                    anchors.right: currentPasswordLabel.left
                },
                AnchorChanges {
                    target: currentPasswordLabel
                    anchors.right: parent.right
                }
            ]
        },
        State {
            name: "landscape"
            when: landscape
            changes: [
                AnchorChanges {
                    target: descriptionLabel
                    anchors.right: prevPasswordLabel.left
                },
                AnchorChanges {
                    target: currentPasswordLabel
                    anchors.right: nextPasswordLabel.left
                }
            ]
        }
    ]
}
