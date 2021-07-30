import QtQuick 2.0
import Sailfish.Silica 1.0
import QtGraphicalEffects 1.0

import "harbour"

Item {
    width: height

    Image {
        id: shadow

        anchors.centerIn: parent
        sourceSize.height: parent.height
        source: "images/jail-black.svg"
        visible: false
    }

    FastBlur {
        source: shadow
        anchors.fill: shadow
        radius: 16
        transparentBorder: true
    }

    HarbourHighlightIcon {
        anchors.fill: shadow
        sourceSize.height: parent.height
        highlightColor: Theme.secondaryColor
        source: "images/jail.svg"
    }
}
