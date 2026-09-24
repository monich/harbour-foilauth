import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: thisItem

    property real minimumValue: 0
    property real maximumValue: 1
    property real value: 0
    property alias text: label.text

    property real _radius: Math.floor(height / 2)
    property real _borderWidth: Math.max(2, Math.floor(Theme.paddingSmall/3))

    implicitHeight: Theme.fontSizeMedium

    Item {
        x: _borderWidth
        y: _borderWidth
        width: (parent.width - 2 * _borderWidth) * Math.max(0, Math.min(1, (value - minimumValue)/maximumValue))
        height: parent.height - 2 * _borderWidth
        clip: true

        Rectangle {
            radius: _radius
            width: thisItem.width - 2 * _borderWidth
            height: parent.height
            color: Theme.rgba(Theme.highlightBackgroundColor, 0.4 /* opacityLow */)
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: _radius
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.2 /* opacityFaint */)
        border {
            color: Theme.rgba(Theme.highlightColor, 0.4 /* opacityLow */)
            width: _borderWidth
        }
    }

    Label {
        id: label

        font.pixelSize: Theme.fontSizeExtraSmall
        anchors.centerIn: parent
    }
}
