import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Page {
    id: thisPage

    property alias sourceModel: selectionModel.sourceModel
    property Item toolPanel

    signal deleteRows(var rows)

    function showHint(text) {
        selectionHint.text = text
        selectionHintTimer.restart()
    }

    function hideHint() {
        selectionHintTimer.stop()
    }

    SilicaFlickable {
        id: flickable

        clip: true
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: toolPanel.top
        }
        PullDownMenu {
            id: pullDownMenu

            onActiveChanged: updateMenuItems()
            function updateMenuItems() {
                if (!active) {
                    selectNoneMenuItem.visible = selectionModel.selectionCount > 0
                    selectAllMenuItem.visible = selectionModel.selectionCount < selectionModel.count
                }
            }
            MenuItem {
                id: selectNoneMenuItem

                //: Pulley menu item
                //% "Deselect all"
                text: qsTrId("foilauth-menu-select_none")
                onClicked: selectionModel.clearSelection()
            }
            MenuItem {
                id: selectAllMenuItem

                //: Pulley menu item, selects all tokens
                //% "Select all"
                text: qsTrId("foilauth-menu-select_all")
                onClicked: selectionModel.selectAll()
            }
        }
        SilicaListView {
            anchors.fill: parent
            model: HarbourSelectionListModel {
                id: selectionModel

                onSelectionCountChanged: pullDownMenu.updateMenuItems()
                onCountChanged: pullDownMenu.updateMenuItems()
            }

            header: PageHeader {
                //: Page title
                //% "Select tokens"
                title: qsTrId("foilauth-select_page-header")
            }

            delegate: ListItem {
                TokenListItem {
                    anchors.fill: parent
                    interactive: false
                    description: model.label
                    prevPassword: model.prevPassword
                    currentPassword: model.currentPassword
                    nextPassword: model.nextPassword
                    favorite: model.favorite
                    hotp: model.type === FoilAuth.TypeHOTP
                    hotpMinus: model.counter > 0
                    landscape: thisPage.isLandscape
                    color: model.selected ? Theme.rgba(Theme.highlightBackgroundColor, 0.2) : "transparent"
                    selected: model.selected
                }
                onClicked: model.selected = !model.selected
            }

            VerticalScrollDecorator { }
        }

        InteractionHintLabel {
            id: selectionHint

            invert: true
            anchors.fill: parent
            visible: opacity > 0
            opacity: selectionHintTimer.running ? 1.0 : 0.0
            Behavior on opacity { FadeAnimation { duration: 1000 } }
        }
    }

    SelectToolPanel {
        id: toolPanel

        readonly property var exportList: sourceModel.generateMigrationUris(selectionModel.selectedRows)

        active: selectionModel.selectionCount > 0
        canExport: exportList.length > 0
        //: Hint text
        //% "Delete selected tokens"
        onShowDeleteHint: thisPage.showHint(qsTrId("foilauth-select_page-hint_delete_selected"))
        //: Hint text
        //% "Export selected tokens via QR code"
        onShowExportHint: thisPage.showHint(qsTrId("foilauth-select_page-hint_export_selected"))
        onDeleteSelectedItems: thisPage.deleteRows(selectionModel.selectedRows)
        onExportSelectedItems: {
            pageStack.push(Qt.resolvedUrl("QRCodeExportPage.qml"), {
                allowedOrientations: thisPage.allowedOrientations,
                mainPage: pageStack.previousPage(thisPage),
                exportList: toolPanel.exportList,
                currentIndex: 0
            })
        }
        onHideHint: thisPage.hideHint()
    }

    Timer {
        id: selectionHintTimer

        interval: 10000
    }
}
