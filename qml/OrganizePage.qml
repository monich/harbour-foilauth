import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: thisPage

    property var foilModel

    readonly property int _fullHeight: thisPage.isPortrait ? Screen.height : Screen.width

    MouseArea {
        height: thisPage.isLandscape ? Theme.itemSizeSmall : Theme.itemSizeLarge
        width: parent.width
        z: scroller.z + 1
    }

    SilicaListView {
        id: scroller

        width: parent.width
        height: Math.min(toolPanel.y, thisPage.height)
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        flickDeceleration: maximumFlickVelocity
        interactive: !scrollAnimation.running
        clip: true

        readonly property real maxContentX: originX + Math.max(0, contentWidth - width)

        model: [ reorderViewComponent, groupsViewComponent ]
        delegate: Loader {
            width: scroller.width
            height: scroller.height
            sourceComponent: modelData
        }
    }

    Component {
        id: reorderViewComponent

        OrganizeReorderView {
            anchors.fill: parent
            foilModel: thisPage.foilModel
            isLandscape: thisPage.isLandscape
        }
    }

    Component {
        id: groupsViewComponent

        OrganizeGroupsView {
            anchors.fill: parent
            // FoilAuthGroupModel slows down reordering, use it only when needed
            foilModel: (scroller.contentX > scroller.originX) ? thisPage.foilModel : null
        }
    }

    OrganizeToolPanel {
        id: toolPanel

        // Position this panel at the bottom of the screen so that it gets
        // hidden behind the keyboard
        y: _fullHeight - height
        highlightReorderButton: scroller.currentIndex === 0
        highlightGroupButtom: scroller.currentIndex === 1
        onReorderTokens: { // Animate positionViewAtBeginning()
            if (!scrollAnimation.running && scroller.contentX > scroller.originX) {
                scrollAnimation.from = scroller.contentX
                scrollAnimation.to = scroller.originX
                scrollAnimation.start()
            }
        }
        onGroupTokens: { // Animate positionViewAtEnd()
            if (!scrollAnimation.running && scroller.contentX < scroller.maxContentX) {
                scrollAnimation.from = scroller.contentX
                scrollAnimation.to = scroller.maxContentX
                scrollAnimation.start()
            }
        }
    }

    Rectangle {
        x: (scroller.contentX - scroller.originX)/2
        y: toolPanel.y
        width: toolPanel.width/2
        height: toolPanel.height
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.1)
    }

    NumberAnimation {
        id: scrollAnimation

        target: scroller
        property: "contentX"
        duration: 200
        easing.type: Easing.InOutQuad
    }
}
