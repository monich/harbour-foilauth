import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Rectangle {
    id: thisItem

    color: "transparent"

    property alias interactive: favoriteButton.visible
    property alias description: descriptionLabel.text
    property string prevPassword
    property string nextPassword
    property string currentPassword
    property bool favorite
    property bool hotp
    property bool hotpMinus
    property bool landscape
    property bool selected

    signal favoriteToggled()
    signal incrementCounter()
    signal decrementCounter()

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
            left: interactive ? favoriteButton.right : parent.left
            leftMargin: interactive ? (landscape ? Theme.paddingLarge : 0) : Theme.horizontalPageMargin
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
        visible: landscape && !hotp
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
        visible: landscape && !hotp
        transform: HarbourTextFlip {
            text: thisItem.nextPassword
            target: nextPasswordLabel
        }
    }

    HarbourIconTextButton {
        id: leftCounterButton

        anchors {
            horizontalCenter: prevPasswordLabel.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        iconSource: hotp ? (landscape ? (hotpMinus ? "images/minus.svg" : "") : "images/plus.svg" ) : ""
        enabled: parent.interactive
        highlighted: down || thisItem.selected
        visible: hotp && currentPassword.length > 0
        onClicked: {
            if (!landscape) {
                thisItem.incrementCounter()
            } else if (hotpMinus) {
                thisItem.decrementCounter()
            }
        }
    }

    HarbourIconTextButton {
        anchors {
            horizontalCenter: nextPasswordLabel.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        iconSource: (landscape && hotp) ? "images/plus.svg" : ""
        enabled: parent.interactive
        highlighted: down || thisItem.selected
        visible: landscape && hotp && currentPassword.length > 0
        onClicked: thisItem.incrementCounter()
    }

    states: [
        State {
            name: "portrait"
            changes: [
                AnchorChanges {
                    target: currentPasswordLabel
                    anchors.right: parent.right
                }
            ]
        },
        State {
            name: "portrait-totp"
            extend: "portrait"
            when: !landscape && !hotp
            changes: [
                AnchorChanges {
                    target: descriptionLabel
                    anchors.right: currentPasswordLabel.left
                }
            ]
        },
        State {
            name: "portrait-hotp"
            extend: "portrait"
            when: !landscape && hotp
            changes: [
                AnchorChanges {
                    target: descriptionLabel
                    anchors.right: leftCounterButton.left
                }
            ]
        },
        State {
            name: "landscape"
            changes: [
                AnchorChanges {
                    target: currentPasswordLabel
                    anchors.right: nextPasswordLabel.left
                }
            ]
        },
        State {
            name: "landscape-totp"
            extend: "landscape"
            when: landscape && !hotp
            changes: [
                AnchorChanges {
                    target: descriptionLabel
                    anchors.right: prevPasswordLabel.left
                }
            ]
        },
        State {
            name: "landscape-hotp"
            extend: "landscape"
            when: landscape && hotp
            changes: [
                AnchorChanges {
                    target: descriptionLabel
                    anchors.right: hotpMinus ? leftCounterButton.left : currentPasswordLabel.left
                }
            ]
        }
    ]
}
