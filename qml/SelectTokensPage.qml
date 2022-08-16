import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

Dialog {
    id: thisDialog

    canAccept: importModel.haveSelectedTokens

    property var tokens

    signal tokensAccepted(var tokens)

    readonly property color _selectionBackground: Theme.rgba(Theme.highlightBackgroundColor, 0.1)

    Component.onCompleted: importModel.setTokens(tokens)

    onAccepted: tokensAccepted(importModel.selectedTokens)

    SilicaListView {
        id: list

        anchors.fill: parent
        width: parent.width
        clip: true
        model: FoilAuthImportModel {
            id: importModel
        }

        header: DialogHeader {
            //: Dialog button
            //% "Save"
            acceptText: qsTrId("foilauth-edit_token-save")
            //: Dialog title
            //% "Select tokens to add"
            title: qsTrId("foilauth-select_tokens-title-add_tokens")
        }

        delegate: ListItem {
            id: delegate

            contentHeight: Theme.itemSizeMedium

            readonly property int itemIndex: model.index
            readonly property bool itemSelected: model.selected

            Separator {
                width: parent.width
                color: Theme.highlightColor
                horizontalAlignment: Qt.AlignHCenter
                anchors.top: parent.top
            }

            Rectangle {
                width: parent.width
                height: delegate.contentHeight
                color: (delegate.itemSelected && !delegate.highlighted) ? _selectionBackground : "transparent"
            }

            Column {
                spacing: Theme.paddingMedium
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: icon.left
                    rightMargin: Theme.paddingLarge
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    color: delegate.itemSelected ? Theme.highlightColor : Theme.secondaryHighlightColor
                    truncationMode: TruncationMode.Fade
                    font.bold: delegate.itemSelected
                    text: model.label
                }

                Label {
                    width: parent.width
                    color: Theme.secondaryHighlightColor
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    text: model.issuer
                    visible: text !== ""
                }
            }

            HarbourHighlightIcon {
                id: icon

                source: model.selected ? "images/checked.svg" :  "images/unchecked.svg"
                sourceSize.width: Theme.iconSizeMedium
                highlightColor: delegate.down ? Theme.highlightColor : Theme.primaryColor
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
            }

            menu: Component {
                ContextMenu {
                    hasContent: delegate.itemSelected
                    MenuItem {
                        //: Generic menu item
                        //% "Edit"
                        text: qsTrId("foilauth-menu-edit")
                        onClicked: {
                            var token = list.model.getToken(delegate.itemIndex)
                            pageStack.push(Qt.resolvedUrl("EditAuthTokenDialog.qml"), {
                                "allowedOrientations": allowedOrientations,
                                "type": token.type,
                                "label": token.label,
                                "issuer": token.issuer,
                                "secret": token.secret,
                                "digits": token.digits,
                                "counter": token.counter,
                                "timeshift": token.timeshift,
                                "algorithm": token.algorithm
                            }).tokenAccepted.connect(delegate.applyChanges)
                        }
                    }
                }
            }

            onClicked: model.selected = !model.selected

            function applyChanges(dialog) {
                // Leave issuer as is
                list.model.setToken(itemIndex, dialog.type, dialog.algorithm,
                    dialog.label, dialog.issuer, dialog.secret, dialog.digits,
                    dialog.counter, dialog.timeshift)
            }
        }
    }
}
