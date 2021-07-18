import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Rectangle {
    id: thisItem

    color: "transparent"

    property alias interactive: favoriteButton.enabled
    property alias description: descriptionLabel.text
    property string prevPassword
    property string nextPassword
    property string currentPassword
    property bool favorite
    property bool landscape
    property bool selected

    signal favoriteToggled()

    HarbourIconTextButton {
        id: favoriteButton

        anchors {
            left: parent.left
            leftMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        iconSource: favorite ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
        highlighted: down || thisItem.selected
        onClicked: thisItem.favoriteToggled()
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
        color: thisItem.selected ? Theme.secondaryHighlightColor : Theme.highlightColor
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
            text: thisItem.prevPassword
            target: prevPasswordLabel
        }
    }

    Label {
        id: currentPasswordLabel

        color: thisItem.selected ? Theme.highlightColor : Theme.primaryColor
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
            text: thisItem.currentPassword
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
            text: thisItem.nextPassword
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
