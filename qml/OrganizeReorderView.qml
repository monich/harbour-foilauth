import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Item {
    id: thisView

    property var foilModel
    property bool isLandscape

    PageHeader {
        id: header

        //: Page header title
        //% "Organize tokens"
        title: qsTrId("foilauth-organize-tokens-title")
        //: Page header descriptions
        //% "Press, hold and drag to reorder"
        description: qsTrId("foilauth-organize-tokens-description")
    }

    SilicaListView {
        id: list

        property var _dragItem
        readonly property real _dragEdge: Theme.horizontalPageMargin
        readonly property real _maxContentY: Math.max(originY + contentHeight - height, originY)

        clip: true
        width: parent.width
        anchors {
            top: header.bottom
            bottom: parent.bottom
        }

        interactive: !Drag.active
        currentIndex: listModel.dragPos
        model: HarbourOrganizeListModel {
            id: listModel

            sourceModel: foilModel
        }

        moveDisplaced: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 50
                easing.type: Easing.InOutQuad
            }
        }

        delegate: ListItem {
            id: listItem

            readonly property bool dragging: list._dragItem === listItem
            readonly property bool _hidden: !model.groupHeader && model.hidden
            readonly property int _modelIndex: index

            height: _hidden ? 0 : implicitHeight
            visible: height > 0

            Behavior on height { SmoothedAnimation { duration: 200 } }

            Item {
                id: draggableItem

                width: listItem.width
                height: listItem.contentHeight
                clip: height < listItem.contentHeight
                scale: listItem.dragging ? 1.05 : 1

                Loader {
                    active: !model.groupHeader
                    anchors.fill: parent
                    sourceComponent: Component {
                        TokenListItem {
                            description: model.label
                            prevPassword: model.prevPassword
                            currentPassword: model.currentPassword
                            nextPassword: model.nextPassword
                            favorite: model.favorite
                            interactive: false
                            hotp: model.type === FoilAuth.TypeHOTP
                            hotpMinus: model.counter > 0
                            landscape: thisView.isLandscape
                            color: listItem.dragging ? Theme.rgba(Theme.highlightBackgroundColor, 0.2) : "transparent"
                            selected: listItem.down
                            enabled: listItem.enabled
                            opacity: listItem._hidden ? 0 : enabled ? 1 : 0.2

                            onFavoriteToggled: model.favorite = !model.favorite
                            onIncrementCounter: {
                                model.counter++
                                copyPassword()
                                buzz()
                            }
                            onDecrementCounter: {
                                model.counter--
                                copyPassword()
                                buzz()
                            }

                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on opacity { FadeAnimation { duration: 200 } }
                        }
                    }
                }

                Loader {
                    active: model.groupHeader
                    anchors.fill: parent
                    sourceComponent: Component {
                        GroupHeader {
                            title: model.label
                            expanded: !model.hidden
                        }
                    }
                }

                Connections {
                    target: listItem.dragging ? list : null
                    onContentYChanged: draggableItem.handleScrolling()
                }

                onYChanged: handleScrolling()

                function handleScrolling() {
                    if (listItem.dragging) {
                        var i = list.indexAt(list.width/2, y + list.contentY + height/2)
                        if (i >= 0 && i !== listItem._modelIndex) {
                            // Swap the items
                            list.model.dragPos = i
                        }
                        var bottom = y + height * 3 / 2
                        if (bottom > list.height) {
                            // Scroll the list down
                            scrollAnimation.stop()
                            scrollAnimation.from = list.contentY
                            scrollAnimation.to = Math.min(list._maxContentY, list.contentY + bottom + height - list.height)
                            if (scrollAnimation.to > scrollAnimation.from) {
                                scrollAnimation.start()
                            }
                        } else {
                            var top = y - height / 2
                            if (top < 0) {
                                // Scroll the list up
                                scrollAnimation.stop()
                                scrollAnimation.from = list.contentY
                                scrollAnimation.to = Math.max(list.originY, list.contentY + top - height)
                                if (scrollAnimation.to < scrollAnimation.from) {
                                    scrollAnimation.start()
                                }
                            }
                        }
                    }
                }

                states: [
                    State {
                        name: "dragging"
                        when: listItem.dragging
                        ParentChange {
                            target: draggableItem
                            parent: list
                            y: draggableItem.mapToItem(list, 0, 0).y
                        }
                    },
                    State {
                        name: "normal"
                        when: !listItem.dragging
                        ParentChange {
                            target: draggableItem
                            parent: listItem.contentItem
                            y: 0
                        }
                    }
                ]

                Behavior on scale {
                    NumberAnimation {
                        duration: 150
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            onClicked: {
                if (model.groupHeader) {
                    model.hidden = !model.hidden
                }
            }

            onPressAndHold: list._dragItem = listItem
            onReleased: stopDrag()
            onCanceled: stopDrag()
            onDraggingChanged: {
                if (dragging) {
                    listModel.dragIndex = index
                }
            }

            function stopDrag() {
                if (list._dragItem === listItem) {
                    list._dragItem = null
                    listModel.dragIndex = -1
                }
            }

            drag.target: dragging ? draggableItem : null
            drag.axis: Drag.YAxis
        }

        VerticalScrollDecorator { }

        NumberAnimation {
            id: scrollAnimation

            target: list
            properties: "contentY"
            duration: 75
            easing.type: Easing.InQuad
        }
    }
}
