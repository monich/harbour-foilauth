import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

CoverBackground {
    id: cover

    readonly property var foilModel: FoilAuthModel
    readonly property int coverActionHeight: Theme.itemSizeSmall
    readonly property string lockIconSource: Qt.resolvedUrl("images/" + (HarbourTheme.darkOnLight ? "cover-lock-dark.svg" :  "cover-lock.svg"))

    Rectangle {
        width: parent.width
        height: appTitle.height
        anchors.top: parent.top
        color: Theme.primaryColor
        opacity: 0.1
    }

    Rectangle {
        id: countdown

        height: appTitle.height
        width: active ? Math.round(value) : cover.width
        color: Theme.primaryColor
        opacity: 0.1

        readonly property bool active: foilModel.timerActive
        property real value

        onActiveChanged: {
            countdownRepeatAnimation.stop()
            countdownInitialAnimation.stop()
            value = cover.width
            if (active) {
                var ms = foilModel.millisecondsLeft()
                if (ms > 1000) {
                    countdownInitialAnimation.duration = ms - 1000
                    countdownInitialAnimation.start()
                }
            }
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
                to: cover.width
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

    Label {
        id: appTitle

        width: parent.width - 2 * Theme.paddingMedium
        anchors.top: parent.top
        height: implicitHeight + Theme.paddingLarge
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        readonly property bool showAppTitle: !list.count || list.currentIndex < 0 || !list.currentLabel
        //: Application title
        //% "Foil Auth"
        text: showAppTitle ? qsTrId("foilauth-app_name") : " "
    }

    Item {
        id: backgroundCircle

        readonly property real size: Math.floor(parent.width * 0.8)
        width: backgroundCircle.size
        height: backgroundCircle.size
        anchors.centerIn: parent
        transform: Rotation {
            id: backgroundCircleRotation

            origin { x: width / 2; y: height / 2 }
            axis { x: 0; y: 1; z: 0 }
        }

        Rectangle {
            anchors.fill: parent
            color: "white"
            radius: height/2
            opacity: 0.2
        }

        HarbourFitLabel {
            id: noPasswordLabel

            width: Math.round(parent.width - 2 * parent.x)
            height: width
            anchors.centerIn: parent
            maxFontSize: Theme.fontSizeHuge
            font {
                family: Theme.fontFamilyHeading
                bold: true
            }
            text: "\u2022\u2022\u2022\u2022\u2022\u2022"
            visible: list.count == 0 && foilModel.keyAvailable
        }
    }

    Image {
        id: lockedImage

        source: "images/foilauth.svg"
        height: backgroundCircle.size
        sourceSize.height: backgroundCircle.size
        anchors.centerIn: parent
        smooth: true
        transform: Rotation {
            id: lockedImageRotation

            origin { x: width / 2; y: height / 2 }
            axis { x: 0; y: 1; z: 0 }
        }
    }

    Connections {
        target: cover.foilModel
        onKeyAvailableChanged: {
            if (cover.foilModel.keyAvailable) {
                // This transition is not visible, there's no reason to animate it
                lockedImage.visible = false
                backgroundCircleRotation.angle = 0
                backgroundCircle.visible = true
            } else {
                lockedImageRotation.angle = 90
                lockFlipAnimation.start()
            }
        }
    }

    // Flip animation
    SequentialAnimation {
        id: lockFlipAnimation

        alwaysRunToEnd: true

        function switchToImage() {
            backgroundCircle.visible = false
            lockedImage.visible = true
        }
        NumberAnimation {
            easing.type: Easing.InOutSine
            target: backgroundCircleRotation
            property: "angle"
            from: 0
            to: 90
            duration: 125
        }
        ScriptAction { script: lockFlipAnimation.switchToImage() }
        NumberAnimation {
            easing.type: Easing.InOutSine
            target: lockedImageRotation
            property: "angle"
            to: 0
            duration: 125
        }
    }

    Timer {
        id: currentIndexTimer

        running: list.count > 1
        interval: 5000
        repeat: true
        onTriggered: list.incrementCurrentIndex()
    }

    SlideshowView {
        id: list

        property string currentLabel
        interactive: false
        anchors.fill: parent
        model: FoilAuthFavoritesModel {
            sourceModel: foilModel
        }
        delegate: Item {
            id: passwordDelegate
            width: parent.width
            height: list.height

            readonly property string modelLabel: model.label
            readonly property bool currentItem: passwordDelegate.PathView.isCurrentItem

            function updateCurrentLabel() {
                if (currentItem) {
                    list.currentLabel = modelLabel
                }
            }

            onCurrentItemChanged: updateCurrentLabel()
            onModelLabelChanged: updateCurrentLabel()
            Component.onCompleted: updateCurrentLabel()

            Label {
                readonly property real maxWidth: parent.width - 2 * Theme.paddingMedium
                width: Math.min(paintedWidth, maxWidth)
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                }
                height: implicitHeight + Theme.paddingLarge
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                truncationMode: TruncationMode.Fade
                text: modelLabel
            }

            HarbourFitLabel {
                id: passwordLabel

                y: Math.round(backgroundCircle.y - list.y + (backgroundCircle.height - height)/2)
                width: Math.round(backgroundCircle.width - 2 * backgroundCircle.x)
                height: width
                anchors.horizontalCenter: parent.horizontalCenter
                maxFontSize: Theme.fontSizeHuge
                font {
                    family: Theme.fontFamilyHeading
                    bold: true
                }
                transform: HarbourTextFlip {
                    text: model.currentPassword
                    target: passwordLabel
                }
            }
        }
    }

    CoverActionList {
        enabled: cover.foilModel.keyAvailable && list.count > 1
        CoverAction {
            iconSource: "image://theme/icon-cover-previous"
            onTriggered: {
                currentIndexTimer.restart()
                list.decrementCurrentIndex()
            }
        }
        CoverAction {
            iconSource: cover.lockIconSource
            onTriggered: cover.foilModel.lock(false)
        }
    }

    CoverActionList {
        enabled: cover.foilModel.keyAvailable && list.count < 2
        CoverAction {
            iconSource: cover.lockIconSource
            onTriggered: cover.foilModel.lock(false)
        }
    }
}
