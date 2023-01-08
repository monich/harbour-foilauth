import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Item {
    id: thisItem

    property int count
    property bool expanded: true
    property alias title: groupHeaderLabel.text

    signal toggle()

    property bool _constructed

    Component.onCompleted: _constructed = true

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.15) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    HarbourBadge {
        id: badge

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        text: count ? count : ""
        opacity: (count > 0 && expanded) ? 1 : 0
        backgroundColor: Theme.rgba(groupHeaderLabel.color, 0.2)
        textColor: groupHeaderLabel.color
    }

    SectionHeader {
        id: groupHeaderLabel

        color: thisItem.highlighted ? Theme.highlightColor : Theme.primaryColor
        anchors {
            left: badge.visible ? badge.right : parent.left
            leftMargin: badge.visible ? Theme.paddingLarge : Theme.horizontalPageMargin
            right: arrow.visible ? arrow.left : parent.right
            rightMargin: arrow.visible ? Theme.paddingMedium : Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        // Label/SectionHeader didn't have "topPadding" in earlier versions of SFOS
        Component.onCompleted: {
            if ('topPadding' in groupHeaderLabel) {
                groupHeaderLabel.topPadding = 0
            }
        }
    }

    HarbourHighlightIcon {
        id: arrow

        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            rightMargin: Theme.paddingMedium
        }
        highlightColor: groupHeaderLabel.color
        source: thisItem.enabled ? "image://theme/icon-m-left" : ""
        visible: thisItem.enabled
    }

    //onClicked: thisItem.toggle()

    states: [
        State {
            when: expanded
            PropertyChanges {
                target: arrow
                rotation: -90
            }
        },
        State {
            when: !expanded
            PropertyChanges {
                target: arrow
                rotation: 0
            }
        }
    ]

    transitions: Transition {
        enabled: _constructed
        to: "*"
        SmoothedAnimation {
            target: arrow
            duration: 200
            property: "rotation"
        }
    }
}
