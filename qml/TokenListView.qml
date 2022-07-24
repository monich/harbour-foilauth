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
    property var disabledItems: []

    readonly property bool isLandscape: mainPage && mainPage.isLandscape
    readonly property real dragEdge: Theme.horizontalPageMargin
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
        SailOTP.importedTokens = FoilAuthSettings.sailotpImportedTokens
        var n = SailOTP.fetchNewTokens(FoilAuthModel)
        if (n > 0) {
            var dialog = pageStack.push(Qt.resolvedUrl("ImportTokensDialog.qml"), {
                allowedOrientations: mainPage.allowedOrientations,
                firstTime: !FoilAuthSettings.sailotpImportDone,
                count: n
            })
            if (dialog) {
                dialog.accepted.connect(function() {
                    FoilAuthSettings.sailotpImportDone = true
                    FoilAuthSettings.sailotpImportedTokens = SailOTP.importedTokens
                    SailOTP.importTokens(FoilAuthModel)
                })
                dialog.rejected.connect(function() {
                    FoilAuthSettings.sailotpImportDone = true
                    FoilAuthSettings.sailotpImportedTokens = SailOTP.importedTokens
                    SailOTP.dropTokens()
                })
            }
        }
    }

    Component {
        id: remorsePopupComponent

        RemorsePopup { }
    }

    Component {
        id: editAuthTokenDialogComponent

        EditAuthTokenDialog {
            allowedOrientations: mainPage.allowedOrientations
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
            //: Pulley menu item, opens selection page
            //% "Select"
            text: qsTrId("foilauth-menu-select_tokens")
            visible: foilModel.count > 0
            onClicked: {
                disabledItems = []
                var selectPage = pageStack.push(Qt.resolvedUrl("SelectPage.qml"), {
                    allowedOrientations: mainPage.allowedOrientations,
                    sourceModel: mainPage.foilModel
                })
                selectPage.deleteRows.connect(function(rows) {
                    disabledItems = foilModel.getIdsAt(rows)
                    pageStack.pop()
                    if (disabledItems.length > 0) {
                        var remorsePopup = remorsePopupComponent.createObject(mainPage)
                        remorsePopup.canceled.connect(function() {
                            disabledItems = []
                            remorsePopup.destroy()
                        })
                        remorsePopup.triggered.connect(function() {
                            // disabledItems will be updated when the remove animation
                            // gets completed so that the item keeps looking disabled
                            // during removal
                            foilModel.deleteTokens(disabledItems)
                            remorsePopup.destroy()
                        })
                        remorsePopup.execute((disabledItems.length === 1) ?
                            //: Remorse popup text (single token selected)
                            //% "Deleting selected token"
                            qsTrId("foilauth-remorse-deleting_selected_token") :
                            //: Remorse popup text (multiple tokens selected)
                            //% "Deleting selected tokens"
                            qsTrId("foilauth-remorse-deleting_selected_tokens"))
                    }
                })
            }
        }
        MenuItem {
            //: Pulley menu item, creates a new authentication token
            //% "Add token"
            text: qsTrId("foilauth-menu-new_auth_token")
            onClicked: {
                pageStack.push("ScanPage.qml", {
                    "allowedOrientations": allowedOrientations
                }).done.connect(function(token) {
                    var props = {
                        //: Dialog button
                        //% "Save"
                        acceptText: qsTrId("foilauth-edit_token-save"),
                        //: Dialog title
                        //% "Add token"
                        dialogTitle: qsTrId("foilauth-add_token-title")
                    }
                    if (token.valid) {
                        props["type"] = token.type
                        props["label"] = token.label
                        props["issuer"] = token.issuer
                        props["secret"] = token.secret
                        props["digits"] = token.digits
                        props["counter"] = token.counter
                        props["timeShift"] = token.timeshift
                        props["algorithm"] = token.algorithm
                    }
                    pageStack.replace(editAuthTokenDialogComponent, props).tokenAccepted.connect(function(dialog) {
                        FoilAuthModel.addToken(dialog.type, dialog.secret, dialog.label, dialog.issuer,
                            dialog.digits, dialog.counter, dialog.timeshift, dialog.algorithm)
                    })
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
            minimumValue: 1
            maximumValue: foilModel.period
            value: foilModel.timeLeft
            visible: opacity > 0
            opacity: foilModel.timerActive ? 1 : 0

            Behavior on opacity { FadeAnimation { } }
            Behavior on value {
                enabled: foilModel.timerActive && Qt.application.active
                NumberAnimation { duration: 500 }
            }
        }
    }

    delegate: ListItem {
        id: tokenListDelegate

        enabled: !disabledItems || !disabledItems.length || disabledItems.indexOf(tokenId) < 0

        readonly property string tokenId: model.modelId
        readonly property int modelIndex: index
        readonly property bool dragging: tokenList.dragItem === tokenListDelegate

        menu: Component {
            ContextMenu {
                MenuItem {
                    //: Context menu item (copy password to clipboard)
                    //% "Copy password"
                    text: qsTrId("foilauth-menu-copy_password")
                    onClicked: Clipboard.text = model.currentPassword
                }
                MenuItem {
                    //: Context menu item
                    //% "Show QR code"
                    text: qsTrId("foilauth-menu-show_qr_code")
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("QRCodePage.qml"), {
                            allowedOrientations: mainPage.allowedOrientations,
                            uri: FoilAuth.toUri(model.type, model.secret, model.label, model.issuer,
                                model.digits, model.counter, model.timeShift, model.algorithm)
                        })
                    }
                }
                MenuItem {
                    //: Generic menu item
                    //% "Edit"
                    text: qsTrId("foilauth-menu-edit")
                    onClicked: {
                        var item  = tokenListDelegate
                        pageStack.push(editAuthTokenDialogComponent, {
                            //: Dialog button
                            //% "Save"
                            acceptText: qsTrId("foilauth-edit_token-save"),
                            //: Dialog title
                            //% "Edit token"
                            dialogTitle: qsTrId("foilauth-edit_token-title"),
                            type: model.type,
                            label: model.label,
                            secret: model.secret,
                            issuer: model.issuer,
                            algorithm: model.algorithm,
                            digits: model.digits,
                            counter: model.counter,
                            timeShift: model.timeShift
                        }).tokenAccepted.connect(function(dialog) {
                            item.updateToken(dialog)
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
                prevPassword: model.prevPassword
                currentPassword: model.currentPassword
                nextPassword: model.nextPassword
                favorite: model.favorite
                hotp: model.type === FoilAuth.TypeHOTP
                hotpMinus: model.counter > 0
                landscape: tokenList.isLandscape
                color: dragging ? Theme.rgba(Theme.highlightBackgroundColor, 0.2) : "transparent"
                selected: tokenListDelegate.down
                enabled: tokenListDelegate.enabled
                opacity: enabled ? 1 : 0.2

                onFavoriteToggled: model.favorite = !model.favorite
                onIncrementCounter: model.counter++
                onDecrementCounter: model.counter--

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
            model.type = token.type
            model.issuer = token.issuer
            model.secret = token.secret
            model.label = token.label
            model.algorithm = token.algorithm
            model.digits = token.digits
            model.counter = token.counter
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

        onClicked: {
            Clipboard.text = model.currentPassword
            clipboardNotification.publish()
            cancelDrag()
        }

        onPressAndHold: cancelDrag()
        onReleased: stopDrag(tokenListDelegate)
        onCanceled: stopDrag(tokenListDelegate)
        onMouseXChanged: {
            // Don't start drag if the initial touch was too close to the edge
            if (!dragging && pressed && !menuOpen && !dragTimer.running &&
                    pressX > tokenList.dragEdge && pressX < (width - tokenList.dragEdge) &&
                    Math.abs(mouseX - pressX) > tokenList.dragThresholdX) {
                startDrag()
            }
        }
        onMouseYChanged: {
            // Don't start drag if the initial touch was too close to the edge
            if (!dragging && pressed && !menuOpen && !dragTimer.running &&
                    pressX > tokenList.dragEdge && pressX < (width - tokenList.dragEdge) &&
                    Math.abs(mouseY - pressY) > tokenList.dragThresholdY) {
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
        ListView.onRemove: RemoveAnimation {
            target: tokenListDelegate
            onRunningChanged: {
                if (!running) {
                    disabledItems = FoilAuth.stringListRemove(disabledItems, tokenId)
                }
            }
        }
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

    VerticalScrollDecorator { }

    ViewPlaceholder {
        enabled: foilModel.count === 0
        //: Placeholder text
        //% "You do not have any encrypted tokens"
        text: qsTrId("foilauth-password_list-placeholder")
    }
}
