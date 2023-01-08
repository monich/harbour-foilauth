import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: thisPage

    property var foilModel

    clip: true

    property bool groupView
    readonly property int _fullHeight: thisPage.isPortrait ? Screen.height : Screen.width

    onWidthChanged: container.updatePosition()
    onGroupViewChanged: container.updatePosition()

    Row {
        id: container

        height: Math.min(toolPanel.y, thisPage.height)

        OrganizeReorderView {
            width: thisPage.width
            height: parent.height
            foilModel: thisPage.foilModel
            isLandscape: thisPage.isLandscape
        }

        OrganizeGroupsView {
            width: thisPage.width
            height: parent.height
            // FoilAuthGroupModel slows down reordering, use it only when needed
            foilModel: (container.x < 0) ? thisPage.foilModel : null
        }

        Behavior on x {
            id: animation

            // We don't want this animation to run on orientation changes
            enabled: false
            SmoothedAnimation {
                duration: 200
                onRunningChanged: if (!running) animation.enabled = false
            }
        }

        function updatePosition() {
            x = groupView ? -thisPage.width : 0
        }
    }

    OrganizeToolPanel {
        id: toolPanel

        // Position this panel at the bottom of the screen so that it gets
        // hidden behind the keyboard
        y: _fullHeight - height
        highlightReorderButton: container.x > -width/2
        highlightGroupButtom: container.x < -width/2
        onReorderTokens: selectGroupView(false)
        onGroupTokens: selectGroupView(true)

        function selectGroupView(value) {
            if (groupView !== value) {
                // One-shot animation
                animation.enabled = true
                groupView = value
            }
        }
    }

    Rectangle {
        x: -container.x/2
        y: toolPanel.y
        width: toolPanel.width/2
        height: toolPanel.height
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.1)
    }
}
