import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

CoverBackground {
    id: cover

    readonly property var foilModel: FoilAuthModel
    readonly property int coverActionHeight: Theme.itemSizeSmall
    readonly property bool darkOnLight: ('colorScheme' in Theme) && Theme.colorScheme === 1
    readonly property string lockIconSource: Qt.resolvedUrl("images/" + (darkOnLight ? "cover-lock-dark.svg" :  "cover-lock.svg"))
    readonly property string displayOn: HarbourSystemState.displayStatus !== HarbourSystemState.MCE_DISPLAY_OFF

    Rectangle {
        width: parent.width
        height: appTitle.height
        anchors.top: parent.top
        color: Theme.primaryColor
        opacity: 0.1
    }

    Rectangle {
        height: appTitle.height
        width: Math.round(parent.width * (active ? value : 1))
        color: Theme.primaryColor
        opacity: 0.1

        readonly property bool active: list.model.needTimer && foilModel.timerActive && displayOn
        readonly property real value: (foilModel.timeLeft - 1)/(foilModel.period - 1)

        Behavior on width {
            enabled: displayOn
            NumberAnimation { duration: 500 }
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
        transform: Scale {
            id: backgroundCircleScale

            origin.x: width / 2
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
        anchors.centerIn: backgroundCircle
        smooth: true
        opacity: 0.8
        Scale {
            id: lockedImageScale

            origin.x: width / 2
        }
    }

    Connections {
        target: cover.foilModel
        onKeyAvailableChanged: {
            if (cover.foilModel.keyAvailable) {
                // This transition is not visible, there's no reason to animate it
                lockedImage.visible = false
                lockedImageScale.xScale = 0
                backgroundCircleScale.xScale = 1
                backgroundCircle.visible = true
            } else {
                lockFlipAnimation.start()
            }
        }
    }

    // Flip animation
    SequentialAnimation {
        id: lockFlipAnimation

        alwaysRunToEnd: true

        function switchToImage() {
            lockedImage.visible = true
            backgroundCircle.visible = false
        }
        NumberAnimation {
            easing.type: Easing.InOutSine
            target: backgroundCircleScale
            property: "xScale"
            from: 1
            to: 0
            duration: 125
        }
        ScriptAction { script: lockFlipAnimation.switchToImage() }
        NumberAnimation {
            easing.type: Easing.InOutSine
            target: lockedImageScale
            property: "xScale"
            from: 0
            to: 1
            duration: 125
        }
    }

    Timer {
        id: currentIndexTimer

        running: list.count > 1 && displayOn
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

    Loader {
        active: HarbourProcessState.jailedApp
        anchors.centerIn: backgroundCircle
        sourceComponent: Component {
            JailDoor {
                anchors.centerIn: parent
                height: backgroundCircle.height + 2 * Theme.paddingLarge
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
