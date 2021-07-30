import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Page {
    id: thisPage

    readonly property string appExe: "harbour-foilauth"
    readonly property string appImage: "images/foilauth.svg"
    readonly property string openReposUrl: "https://openrepos.net/content/slava/foil-auth"
    readonly property string gitHubUrl: "https://github.com/monich/harbour-foilauth/releases"

    Item {
        id: infoPanel

        anchors {
            bottom: parent.bottom
            right: parent.right
            topMargin: Theme.paddingLarge
            bottomMargin: Theme.paddingLarge
            rightMargin: Theme.horizontalPageMargin
        }

        Column {
            id: infoText

            width: parent.width
            spacing: Theme.paddingLarge

            Label {
                width: parent.width
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
                linkColor: Theme.highlightColor
                //: Label text explaining the sandbox situation
                //% "This application is not designed to function in a sandbox. Try installing the latest version from <b><a href='%1'>OpenRepos</a></b> or <b><a href='%2'>GitHub</a></b>, it may implement some sort of a workaround. No guarantee, though."
                text: qsTrId("jail-explanation").arg(openReposUrl).arg(gitHubUrl)
                onLinkActivated: Qt.openUrlExternally(link)
            }

            Label {
                width: parent.width
                color: Theme.secondaryColor
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeSmall
                //: Hint suggesting to run the app from the terminal
                //% "If you have developer mode enabled, you may also try running <b>%1</b> from the terminal. Sorry for the inconvenience!"
                text: qsTrId("jail-terminal_hint").arg(appExe)
            }
        }

        HarbourHighlightIcon {
            y: Math.round((infoText.y + infoText.height + parent.height - height)/2)
            visible: y > (infoText.y + infoText.height + infoText.spacing)
            anchors.horizontalCenter: parent.horizontalCenter
            sourceSize.height: Theme.fontSizeLarge
            highlightColor: Theme.secondaryColor
            source: "images/shrug.svg"
        }
    }

    Item {
        id: graphics

        anchors.topMargin: Theme.paddingLarge
        height: Math.min(parent.width, parent.height) - 2 * Theme.paddingLarge
        width: height

        Image {
            anchors.centerIn: parent
            sourceSize.height: Theme.itemSizeHuge
            source: appImage
        }

        Flickable {
            anchors.fill: parent
            contentWidth: width
            contentHeight: height
            boundsBehavior: Flickable.DragAndOvershootBounds
            flickableDirection: Flickable.HorizontalAndVerticalFlick

            JailDoor {
                anchors.centerIn: parent
                height: parent.height
            }
        }
    }

    states: [
        State {
            name: "portrait"
            when: !isLandscape
            changes: [
                AnchorChanges {
                    target: graphics
                    anchors {
                        top: thisPage.top
                        verticalCenter: undefined
                        horizontalCenter: thisPage.horizontalCenter
                    }
                },
                AnchorChanges {
                    target: infoPanel
                    anchors {
                        top: graphics.bottom
                        left: thisPage.left
                    }
                },
                PropertyChanges {
                    target: infoPanel
                    anchors.leftMargin: Theme.horizontalPageMargin
                }
            ]
        },
        State {
            name: "landscape"
            when: isLandscape
            changes: [
                AnchorChanges {
                    target: graphics
                    anchors {
                        left: thisPage.left
                        verticalCenter: thisPage.verticalCenter
                        horizontalCenter: undefined
                    }
                },
                AnchorChanges {
                    target: infoPanel
                    anchors {
                        top: thisPage.top
                        left: graphics.right
                    }
                },
                PropertyChanges {
                    target: infoPanel
                    anchors.leftMargin: 0
                }
            ]
        }
    ]
}
