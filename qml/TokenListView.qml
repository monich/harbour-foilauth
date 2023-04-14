import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import harbour.foilauth 1.0

import "foil-ui"
import "harbour"

Item {
    id: thisItem

    property Page mainPage
    property var foilUi
    property var foilModel
    property var disabledItems: []

    readonly property bool _isLandscape: mainPage && mainPage.isLandscape

    SilicaListView {
        id: tokenList

        anchors.fill: parent

        model: HarbourOrganizeListModel {
            id: listModel

            sourceModel: foilModel
        }

        Loader {
            id: buzzLoader

            active: false
            source: Qt.resolvedUrl("Buzz.qml")
        }

        function buzz() {
            // Load buzzer on demand
            buzzLoader.active = true
            if (buzzLoader.item) {
                buzzLoader.item.play()
            }
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
                    mainPage: mainPage,
                    foilUi: thisItem.foilUi,
                    foilModel: thisItem.foilModel,
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
                //: Pulley menu item, opens organize page
                //% "Organize"
                text: qsTrId("foilauth-menu-organize")
                visible: foilModel.count > 0
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("OrganizePage.qml"), {
                        allowedOrientations: mainPage.allowedOrientations,
                        foilModel: mainPage.foilModel
                    })
                }
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
                        foilModel: mainPage.foilModel
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
                    var page = pageStack.push("ScanPage.qml", {
                        "allowedOrientations": allowedOrientations
                    })

                    page.skip.connect(function() {
                        pageStack.replace(editAuthTokenDialogComponent, {
                            //% "Add token"
                            "dialogTitle": qsTrId("foilauth-add_token-title")
                        }).tokenAccepted.connect(function(dialog) {
                            FoilAuthModel.addToken(dialog.type, dialog.secret, dialog.label, dialog.issuer,
                                dialog.digits, dialog.counter, dialog.timeshift, dialog.algorithm)
                        })
                    })
                    page.tokenDetected.connect(function(token) {
                        pageStack.replace(editAuthTokenDialogComponent, {
                            //: Dialog title
                            //% "Add token"
                            "dialogTitle": qsTrId("foilauth-add_token-title"),
                            "type": token.type,
                            "label": token.label,
                            "issuer": token.issuer,
                            "secret": token.secret,
                            "digits": token.digits,
                            "counter": token.counter,
                            "timeshift": token.timeshift,
                            "algorithm": token.algorithm
                        }).tokenAccepted.connect(function(dialog) {
                            FoilAuthModel.addToken(dialog.type, dialog.secret, dialog.label, dialog.issuer,
                                dialog.digits, dialog.counter, dialog.timeshift, dialog.algorithm)
                        })
                    })
                    page.tokensDetected.connect(function(model) {
                        pageStack.replace(Qt.resolvedUrl("SelectTokensPage.qml"), {
                            "allowedOrientations": thisPage.allowedOrientations,
                            "tokens": model.getTokens()
                        }).tokensAccepted.connect(FoilAuthModel.addTokens)
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
            id: listItem

            readonly property int modelIndex: index
            readonly property string tokenId: model.modelId
            readonly property bool _hidden: !model.groupHeader && model.hidden

            showMenuOnPressAndHold: !model.groupHeader
            enabled: !disabledItems || !disabledItems.length || disabledItems.indexOf(tokenId) < 0
            height: _hidden ? 0 : implicitHeight
            clip: height < implicitHeight
            visible: height > 0

            Behavior on height {
                enabled: !listItem.menuOpen
                SmoothedAnimation { duration: 200 }
            }

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
                                label: model.label,
                                issuer: model.issuer,
                                uri: FoilAuth.toUri(model.type, model.secret, model.label, model.issuer,
                                    model.digits, model.counter, model.timeshift, model.algorithm)
                            })
                        }
                    }
                    MenuItem {
                        //: Generic menu item
                        //% "Edit"
                        text: qsTrId("foilauth-menu-edit")
                        onClicked: {
                            var item  = listItem
                            pageStack.push(editAuthTokenDialogComponent, {
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
                                timeshift: model.timeshift
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
                            var tokenId = listItem.tokenId
                            //: Remorse popup text
                            //% "Deleting"
                            remorseAction(qsTrId("foilauth-menu-delete_remorse"), function() {
                                model.deleteToken(tokenId)
                            })
                        }
                    }
                }
            }

            Loader {
                active: !model.groupHeader
                width: listItem.width
                height: listItem.contentHeight
                sourceComponent: Component {
                    TokenListItem {
                        description: model.label
                        prevPassword: model.prevPassword
                        currentPassword: model.currentPassword
                        nextPassword: model.nextPassword
                        favorite: model.favorite
                        hotp: model.type === FoilAuth.TypeHOTP
                        hotpMinus: model.counter > 0
                        landscape: _isLandscape
                        selected: listItem.down
                        enabled: listItem.enabled
                        opacity: listItem._hidden ? 0 : enabled ? 1 : 0.2

                        onFavoriteToggled: model.favorite = !model.favorite
                        onIncrementCounter: {
                            model.counter++
                            listItem.copyPassword()
                            buzz()
                        }
                        onDecrementCounter: {
                            model.counter--
                            listItem.copyPassword()
                            buzz()
                        }

                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on opacity { FadeAnimation { duration: 200 } }
                    }
                }
            }

            Loader {
                active: model.groupHeader
                width: listItem.width
                height: listItem.contentHeight
                sourceComponent: Component {
                    GroupHeader {
                        title: model.label
                        expanded: !model.hidden
                    }
                }
            }

            function copyPassword() {
                Clipboard.text = model.currentPassword
                clipboardNotification.publish()
            }

            function updateToken(token) {
                model.type = token.type
                model.issuer = token.issuer
                model.secret = token.secret
                model.label = token.label
                model.algorithm = token.algorithm
                model.digits = token.digits
                model.counter = token.counter
                model.timeshift = token.timeshift
            }

            onClicked: {
                if (model.groupHeader) {
                    model.hidden = !model.hidden
                } else {
                    copyPassword()
                }
            }

            ListView.onAdd: AddAnimation { target: listItem }
            ListView.onRemove: RemoveAnimation {
                target: listItem
                onRunningChanged: {
                    if (!running) {
                        disabledItems = FoilAuth.stringListRemove(disabledItems, tokenId)
                    }
                }
            }
        }

        VerticalScrollDecorator { }

        ViewPlaceholder {
            enabled: foilModel.count === 0
            //: Placeholder text
            //% "You do not have any encrypted tokens"
            text: qsTrId("foilauth-password_list-placeholder")
        }
    }
}
