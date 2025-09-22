import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Item {
    id: thisView

    property alias foilModel: groupModel.sourceModel

    SilicaListView {
        id: list

        property var _editItem

        clip: true
        anchors.fill: parent

        model: FoilAuthGroupModel {
            id: groupModel
        }

        header: PageHeader {
            //: Page header title
            //% "Manage groups"
            title: qsTrId("foilauth-organize-groups-title")
            //: Page header descriptions
            //% "Create, delete and rename groups"
            description: qsTrId("foilauth-organize-groups-description")
        }

        delegate: ListItem {
            id: listItem

            readonly property string modelId: model.modelId

            width: parent.width

            menu: Component {
                ContextMenu {
                    MenuItem {
                        //: Context menu item
                        //% "Rename"
                        text: qsTrId("foilauth-organize-groups-menu-rename")
                        onClicked: listItem.startEditing()
                    }
                    MenuItem {
                        //: Context menu item
                        //% "Delete"
                        text: qsTrId("foilauth-organize-groups-menu-delete")
                        onClicked: listItem.remove()
                    }
                }
            }

            function remove() {
                //: Remorse item label
                //% "Deleting"
                remorseAction(qsTrId("foilauth-organize-groups-remorse-deleting"), function() {
                    foilModel.deleteGroupItem(listItem.modelId)
                })
            }

            function startEditing() {
                groupItem.editText = model.label
                list._editItem = listItem
                groupItem.startEditing()
            }

            GroupListItem {
                id: groupItem

                width: listItem.width
                height: listItem.contentHeight
                groupName: model.label
                highlighted: listItem.highlighted
                editing: list._editItem === listItem

                onDoneEditing: {
                    list._editItem = null
                    if (editText.length > 0) {
                        model.label = editText
                    }
                }
            }

            ListView.onAdd: AddAnimation { target: listItem }
            ListView.onRemove: RemoveAnimation { target: listItem }
        }

        footer: BackgroundItem {
            id: addGroupItem

            Image {
                id: addIcon

                x: Theme.horizontalPageMargin - Theme.paddingSmall
                anchors.verticalCenter: parent.verticalCenter
                source: "image://theme/icon-m-add" + (addGroupItem.highlighted ? "?" + Theme.highlightColor : "")
            }

            Label {
                //: List footer button label
                //% "Add group"
                text: qsTrId("foilauth-organize-groups-add_group")
                anchors {
                    left: addIcon.right
                    leftMargin: Theme.paddingSmall
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                }
                color: addGroupItem.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            onClicked: {
                //: Default name for the new group
                //% "New group"
                foilModel.addGroup(qsTrId("foilauth-organize-groups-new_group"))
                list.currentIndex = groupModel.count - 1
                list.currentItem.startEditing()
            }
        }

        VerticalScrollDecorator { }
    }
}
