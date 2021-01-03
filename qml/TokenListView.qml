import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import harbour.foilauth 1.0

import "foil-ui"
import "harbour"

SilicaListView {
    id: tokenList

    property Page mainPage
    property var foilUi
    property var foilModel
    property var dragItem

    readonly property bool isLandscape: mainPage && mainPage.isLandscape
    readonly property real dragThresholdX: Theme.itemSizeSmall/2
    readonly property real dragThresholdY: Theme.itemSizeSmall/5
    readonly property real maxContentY: Math.max(originY + contentHeight - height, originY)

    anchors.fill: parent
    interactive: !scrollAnimation.running
    currentIndex: listModel.dragPos

    model: HarbourOrganizeListModel {
        id: listModel

        sourceModel: foilModel
    }

    Notification {
        id: clipboardNotification

        //: Pop-up notification
        //% "Password copied to clipboard"
        previewBody: qsTrId("foilauth-notification-copied_to_clipboard")
        expireTimeout: 2000
        Component.onCompleted: {
            if ("icon" in clipboardNotification) {
                clipboardNotification.icon = "icon-s-clipboard"
            }
        }
    }

    Component.onCompleted: {
        if (!FoilAuthSettings.sailotpImportDone) {
            var n = SailOTP.fetchTokens()
            if (n > 0) {
                FoilAuthSettings.sailotpImportDone = true
                var dialog = pageStack.push(Qt.resolvedUrl("ImportTokensDialog.qml"), {
                    count: n
                })
                if (dialog) {
                    dialog.accepted.connect(function() {
                        SailOTP.importTokens(FoilAuthModel)
                    })
                    dialog.rejected.connect(function() {
                        SailOTP.dropTokens()
                    })
                }
            }
        }
    }

    PullDownMenu {
        MenuItem {
            //: Pulley menu item, changes Foil password
            //% "Change password"
            text: qsTrId("foilauth-menu-change_foil_password")
            onClicked: pageStack.push(Qt.resolvedUrl("foil-ui/FoilUiChangePasswordPage.qml"), {
                mainPage: tokenList.mainPage,
                foilUi: tokenList.foilUi,
                foilModel: tokenList.foilModel,
                //: Password change prompt
                //% "Please enter the current and the new password"
                promptText: qsTrId("foilauth-change_password_page-label-enter_passwords"),
                //: Placeholder and label for the current password prompt
                //% "Current password"
                currentPasswordLabel: qsTrId("foilauth-change_password_page-text_field_label-current_password"),
                //: Placeholder and label for the new password prompt
                //% "New password"
                newPasswordLabel: qsTrId("foilauth-change_password_page-text_field_label-new_password"),
                //: Button label
                //% "Change password"
                buttonText: qsTrId("foilauth-change_password_page-button-change_password")
            })
        }
        MenuItem {
            //: Pulley menu item, locks the tokens
            //% "Lock"
            text: qsTrId("foilauth-menu-lock")
            onClicked: foilModel.lock(false)
        }
        MenuItem {
            //: Pulley menu item, deletes all tokens
            //% "Delete all"
            text: qsTrId("foilauth-menu-delete_all_tokens")
            visible: foilModel.count > 0
            onClicked: {
                //: Remorse popup text
                //% "Deleting all tokens"
                remorsePopup.execute(qsTrId("foilauth-remorse-delete_all_tokens"),
                    function() { foilModel.deleteAll() })
            }
        }
        MenuItem {
            id: newNoteMenuItem

            //: Pulley menu item, creates a new authentication token
            //% "Add token"
            text: qsTrId("foilauth-menu-new_auth_token")
            onClicked: {
                var editPage = pageStack.push(Qt.resolvedUrl("EditAuthTokenDialog.qml"), {
                    //: Dialog button
                    //% "Save"
                    acceptText: qsTrId("foilauth-edit_token-save"),
                    //: Dialog title
                    //% "Add token"
                    dialogTitle: qsTrId("foilauth-add_token-title"),
                    canScan: true
                })
                editPage.accepted.connect(function() {
                    FoilAuthModel.addToken(editPage.secret, editPage.label,
                        editPage.issuer, editPage.digits, editPage.timeshift)
                })
            }
        }
    }

    header: PageHeader {
        id: header

        //: Application title
        //% "Foil Auth"
        title: qsTrId("foilauth-app_name")
        leftMargin: 0

        ProgressBar {
            id: countdown

            x: header.extraContent.x
            width: header.extraContent.width
            anchors.verticalCenter: header.extraContent.verticalCenter
            leftMargin: Theme.horizontalPageMargin + Theme.paddingMedium
            rightMargin: header.rightMargin
            minimumValue: 0
            maximumValue: foilModel.period
            visible: opacity > 0
            opacity: active ? 1 : 0

            readonly property bool active: foilModel.timerActive

            Behavior on opacity { FadeAnimation { } }

            onActiveChanged: {
                countdownRepeatAnimation.stop()
                countdownInitialAnimation.stop()
                if (active) {
                    var ms = foilModel.millisecondsLeft()
                    if (ms > 1000) {
                        value = ms / 1000
                        countdownInitialAnimation.duration = ms - 1000
                        countdownInitialAnimation.start()
                        return
                    }
                }
                value = 0
            }

            Connections {
                target: countdown.active ? foilModel : null
                onTimerRestarted: {
                    countdownInitialAnimation.stop()
                    countdownRepeatAnimation.restart()
                }
            }

            NumberAnimation on value {
                id: countdownInitialAnimation

                to: 0
                easing.type: Easing.Linear
            }

            SequentialAnimation {
                id: countdownRepeatAnimation

                NumberAnimation {
                    target: countdown
                    property: "value"
                    to: foilModel.period
                    easing.type: Easing.Linear
                    duration: 1000
                }
                NumberAnimation {
                    target: countdown
                    property: "value"
                    to: 0
                    easing.type: Easing.Linear
                    duration: (foilModel.period - 2) * 1000
                }
            }
        }
    }

    delegate: ListItem {
        id: tokenListDelegate

        readonly property string tokenId: model.modelId
        readonly property string modelPrevPassword: model.prevPassword
        readonly property int modelIndex: index
        readonly property bool dragging: tokenList.dragItem === tokenListDelegate

        menu: Component {
            ContextMenu {
                MenuItem {
                    //: Context menu item (copy to clipboard)
                    //% "Copy"
                    text: qsTrId("foilauth-menu-copy")
                    onClicked: {
                        Clipboard.text = model.currentPassword
                        clipboardNotification.publish()
                    }
                }
                MenuItem {
                    //: Context menu item
                    //% "Show QR code"
                    text: qsTrId("foilauth-menu-show_qr_code")
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("QRCodePage.qml"), {
                            allowedOrientations: mainPage.allowedOrientations,
                            uri: FoilAuth.toUri(model.secret, model.label, model.issuer, model.digits, model.timeShift)
                        })
                    }
                }
                MenuItem {
                    //: Generic menu item
                    //% "Edit"
                    text: qsTrId("foilauth-menu-edit")
                    onClicked: {
                        var item  = tokenListDelegate
                        var editPage = pageStack.push(Qt.resolvedUrl("EditAuthTokenDialog.qml"), {
                            //: Dialog button
                            //% "Save"
                            acceptText: qsTrId("foilauth-edit_token-save"),
                            //: Dialog title
                            //% "Edit token"
                            dialogTitle: qsTrId("foilauth-edit_token-title"),
                            label: model.label,
                            secret: model.secret,
                            issuer: model.issuer,
                            digits: model.digits,
                            timeShift: model.timeShift
                        })
                        editPage.accepted.connect(function() {
                            item.updateToken(editPage)
                        })
                    }
                }
                MenuItem {
                    //: Generic menu item
                    //% "Delete"
                    text: qsTrId("foilauth-menu-delete")
                    onClicked: {
                        var model = foilModel
                        var tokenId = tokenListDelegate.tokenId
                        //: Remorse popup text
                        //% "Deleting"
                        remorseAction(qsTrId("foilauth-menu-delete_remorse"), function() {
                            model.deleteToken(tokenId)
                        })
                    }
                }
            }
        }

        Item {
            id: draggableItem

            width: tokenListDelegate.width
            height: tokenListDelegate.contentHeight
            scale: dragging ? 1.05 : 1

            TokenListItem {
                anchors.fill: parent
                description: model.label
                prevPassword: modelPrevPassword
                currentPassword: model.currentPassword
                nextPassword: model.nextPassword
                favorite: model.favorite
                landscape: tokenList.isLandscape
                color: dragging ? Theme.rgba(Theme.highlightBackgroundColor, 0.2) : "transparent"

                onFavoriteToggled: model.favorite = !model.favorite

                Behavior on color { ColorAnimation { duration: 150 } }
            }

            Connections {
                target: tokenListDelegate.dragging ? tokenList : null
                onContentYChanged: draggableItem.handleScrolling()
            }

            onYChanged: handleScrolling()

            function handleScrolling() {
                if (dragging) {
                    var i = Math.floor((y + tokenList.contentY + height/2)/height)
                    if (i >= 0 && i !== modelIndex) {
                        // Swap the items
                        tokenList.model.dragPos = i
                    }
                    var bottom = y + height * 3 / 2
                    if (bottom > tokenList.height) {
                        // Scroll the list down
                        scrollAnimation.stop()
                        scrollAnimation.from = tokenList.contentY
                        scrollAnimation.to = Math.min(tokenList.maxContentY, tokenList.contentY + bottom + height - tokenList.height)
                        if (scrollAnimation.to > scrollAnimation.from) {
                            scrollAnimation.start()
                        }
                    } else {
                        var top = y - height / 2
                        if (top < 0) {
                            // Scroll the list up
                            scrollAnimation.stop()
                            scrollAnimation.from = tokenList.contentY
                            scrollAnimation.to = Math.max(tokenList.originY, tokenList.contentY + top - height)
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
                    when: dragging
                    ParentChange {
                        target: draggableItem
                        parent: tokenList
                        y: draggableItem.mapToItem(tokenList, 0, 0).y
                    }
                },
                State {
                    name: "normal"
                    when: !dragging
                    ParentChange {
                        target: draggableItem
                        parent: tokenListDelegate.contentItem
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

        function updateToken(token) {
            model.issuer = token.issuer
            model.secret = token.secret
            model.label = token.label
            //model.issuer = token.issuer
            model.digits = token.digits
            model.timeShift = token.timeShift
        }

        property real pressX
        property real pressY

        onPressed: {
            tokenList.cancelDrag()
            dragTimer.restart()
            pressX = mouseX
            pressY = mouseY
        }

        onClicked: cancelDrag()
        onPressAndHold: cancelDrag()
        onReleased: stopDrag(tokenListDelegate)
        onCanceled: stopDrag(tokenListDelegate)
        onMouseXChanged: {
            if (!dragging && pressed && !menuOpen && !dragTimer.running && Math.abs(mouseX - pressX) > tokenList.dragThresholdX) {
                startDrag()
            }
        }
        onMouseYChanged: {
            if (!dragging && pressed && !menuOpen && !dragTimer.running && Math.abs(mouseY - pressY) > tokenList.dragThresholdY) {
                startDrag()
            }
        }

        function startDrag(item) {
            dragItem = tokenListDelegate
            listModel.dragIndex = modelIndex
        }

        drag.target: dragging ? draggableItem : null
        drag.axis: Drag.YAxis

        ListView.onAdd: AddAnimation { target: tokenListDelegate }
        ListView.onRemove: RemoveAnimation { target: tokenListDelegate }
    }

    moveDisplaced: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 50
            easing.type: Easing.InOutQuad
        }
    }

    NumberAnimation {
        id: scrollAnimation

        target: tokenList
        properties: "contentY"
        duration: 75
        easing.type: Easing.InQuad
    }

    Timer {
        id: dragTimer
        interval: 150
    }

    function stopDrag(item) {
        if (dragItem === item) {
            dragItem = null
            listModel.dragIndex = -1
        }
    }

    function cancelDrag() {
        dragItem = null
        listModel.dragIndex = -1
        dragTimer.stop()
    }

    RemorsePopup {
        id: remorsePopup
    }

    VerticalScrollDecorator { }

    ViewPlaceholder {
        enabled: foilModel.count === 0
        //: Placeholder text
        //% "You do not have any encrypted tokens"
        text: qsTrId("foilauth-password_list-placeholder")
    }
}
