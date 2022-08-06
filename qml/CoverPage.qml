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

    Flipable {
        id: flipable

        readonly property real circleSize: Math.floor(parent.width * 0.8)
        readonly property bool flipped: cover.foilModel.keyAvailable
        property bool flipping
        property real targetAngle

        anchors.fill: parent

        front: Item {
            anchors.fill: parent

            Image {
                source: "images/foilauth.svg"
                height: flipable.circleSize
                sourceSize.height: flipable.circleSize
                anchors.centerIn: parent
                smooth: true
                opacity: 0.8
            }
        }

        back: Item {
            anchors.fill: parent

            Rectangle {
                id: backgroundCircle

                width: flipable.circleSize
                height: flipable.circleSize
                anchors.centerIn: parent
                color: "white"
                radius: flipable.circleSize/2
                opacity: 0.2
            }

            HarbourFitLabel {
                width: Math.round(backgroundCircle.width - 2 * parent.x)
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

                    Component.onCompleted: updateCurrentLabel()
                    onCurrentItemChanged: updateCurrentLabel()
                    onModelLabelChanged: updateCurrentLabel()

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
                            enabled: displayOn
                            text: model.currentPassword
                            target: passwordLabel
                        }
                    }
                }
            }
        }

        transform: Rotation {
            id: rotation

            origin {
                x: flipable.width/2
                y: flipable.height/2
            }
            axis {
                x: 0
                y: 1
                z: 0
            }
        }

        states: [
            State {
                name: "front"
                when: !flipable.flipped
                PropertyChanges {
                    target: rotation
                    angle: flipable.targetAngle
                }
            },
            State {
                name: "back"
                when: flipable.flipped
                PropertyChanges {
                    target: rotation
                    angle: 180
                }
            }
        ]

        transitions: Transition {
            SequentialAnimation {
                ScriptAction { script: flipable.flipping = true; }
                NumberAnimation {
                    target: rotation
                    property: "angle"
                    duration: 500
                }
                ScriptAction { script: flipable.completeFlip() }
            }
        }

        onFlippedChanged: {
            if (!flipped) {
                targetAngle = 360
            }
        }

        function completeFlip() {
            flipping = false
            if (!flipped) {
                targetAngle = 0
            }
        }
    }

    Timer {
        id: currentIndexTimer

        running: list.count > 1 && displayOn
        interval: 5000
        repeat: true
        onTriggered: list.incrementCurrentIndex()
    }

    Loader {
        active: HarbourProcessState.jailedApp
        anchors.centerIn: flipable
        sourceComponent: Component {
            JailDoor {
                anchors.centerIn: parent
                height: flipable.height + 2 * Theme.paddingLarge
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
