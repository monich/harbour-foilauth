import QtQuick 2.0
import Sailfish.Silica 1.0

InteractionHintLabel {
    anchors.fill: parent
    invert: true
    visible: opacity > 0
    opacity: 0.0

    Behavior on opacity { FadeAnimation { duration: 1000 } }
}
