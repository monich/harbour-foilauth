import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

CoverBackground {
    id: cover

    readonly property var foilModel: FoilAuthModel

    readonly property bool _darkOnLight: ('colorScheme' in Theme) && Theme.colorScheme === 1
    readonly property string _lockIconSource: Qt.resolvedUrl("images/" + (_darkOnLight ? "cover-lock-dark.svg" :  "cover-lock.svg"))
    readonly property string _displayOn: !HarbourSystemState.displayOff

    Rectangle {
        width: parent.width
        height: appTitle.height
        anchors.top: parent.top
        color: Theme.rgba(Theme.primaryColor, 0.1)

        Label {
            id: appTitle

            width: parent.width - 2 * Theme.paddingMedium
            anchors.top: parent.top
            height: implicitHeight + Theme.paddingLarge
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            //: Application title
            //% "Foil Auth"
            text: qsTrId("foilauth-app_name")
            opacity: (!list.count || list.currentIndex < 0 || !list.currentLabel || flipable.flipping) ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { FadeAnimation { duration: 500 } }
        }
    }

    Connections {
        target: foilModel
        onKeyAvailableChanged: flipable.flipped = foilModel.keyAvailable
    }

    Flipable {
        id: flipable

        readonly property real circleSize: Math.floor(parent.width * 0.8)
        property bool flipped
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

            ProgressCircle {
                opacity: (list.currentItem &&  list.currentItem.itemType !== FoilAuth.TypeHOTP) ? 1 : 0
                anchors.fill: backgroundCircle
                value: 1.0 - foilModel.timeLeft / FoilAuth.PERIOD
                progressColor: Theme.rgba(Theme.highlightBackgroundColor, 0.2 /* opacityFaint */)
                backgroundColor: Theme.rgba(Theme.highlightColor, 0.4 /* opacityLow */)

                Behavior on opacity { FadeAnimation { } }
                Behavior on value { NumberAnimation { duration: 500 } }
            }

            Label {
                width: Math.round(backgroundCircle.width - 2 * parent.x)
                height: width
                color: Theme.highlightColor
                anchors {
                    margins: Theme.paddingMedium
                    centerIn: parent
                }
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                fontSizeMode: Text.Fit
                minimumPixelSize: Theme.fontSizeTiny
                font {
                    family: Theme.fontFamilyHeading
                    pixelSize: Theme.fontSizeHuge
                    bold: true
                }
                text: "\u2022\u2022\u2022\u2022\u2022\u2022"
                visible: list.count == 0 && foilModel.keyAvailable
            }

            SlideshowView {
                id: list

                property string currentLabel: currentItem ? currentItem.itemLabel : ""

                interactive: false
                anchors.fill: parent
                cacheItemCount: count
                clip: true

                model: FoilAuthFavoritesModel {
                    sourceModel: foilModel
                }

                delegate: Item {
                    id: passwordDelegate

                    width: parent.width
                    height: list.height

                    readonly property int itemType: model.type
                    readonly property string itemLabel: model.label
                    readonly property bool currentItem: passwordDelegate.PathView.isCurrentItem

                    Label {
                        readonly property real maxWidth: parent.width - 2 * Theme.paddingMedium

                        visible: !flipable.flipping
                        width: Math.min(paintedWidth, maxWidth)
                        anchors {
                            top: parent.top
                            horizontalCenter: parent.horizontalCenter
                        }
                        height: implicitHeight + Theme.paddingLarge
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        truncationMode: TruncationMode.Fade
                        text: itemLabel
                    }

                    Label {
                        id: passwordLabel

                        y: Math.round(backgroundCircle.y - list.y + (backgroundCircle.height - height)/2)
                        width: Math.round(backgroundCircle.width - 2 * backgroundCircle.x)
                        height: width
                        anchors {
                            margins: Theme.paddingMedium
                            horizontalCenter: parent.horizontalCenter
                        }
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        color: Theme.highlightColor
                        fontSizeMode: Text.Fit
                        minimumPixelSize: Theme.fontSizeTiny
                        font {
                            family: Theme.fontFamilyHeading
                            pixelSize: Theme.fontSizeHuge
                            bold: true
                        }
                        transform: HarbourTextFlip {
                            enabled: _displayOn
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
                foilModel.lock(false)
            }
        }
    }

    Timer {
        id: currentIndexTimer

        running: list.count > 1 && _displayOn
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
        enabled: foilModel.keyAvailable && list.count > 1

        CoverAction {
            iconSource: "image://theme/icon-cover-previous"
            onTriggered: {
                currentIndexTimer.restart()
                list.decrementCurrentIndex()
            }
        }

        CoverAction {
            iconSource: cover._lockIconSource
            onTriggered: flipable.flipped = false
        }
    }

    CoverActionList {
        enabled: foilModel.keyAvailable && list.count < 2

        CoverAction {
            iconSource: cover._lockIconSource
            onTriggered: flipable.flipped = false
        }
    }
}
