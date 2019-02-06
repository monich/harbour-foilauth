import QtQuick 2.0
import QtMultimedia 5.4
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import harbour.foilauth 1.0

Page {
    id: page

    allowedOrientations: appAllowedOrientations

    property Item viewFinder
    property Item hint

    readonly property string imageProvider: HarbourTheme.darkOnLight ? HarbourImageProviderDarkOnLight : HarbourImageProvider
    readonly property string iconSourcePrefix: "image://" + imageProvider + "/"
    readonly property bool canShowViewFinder: Qt.application.active && page.status === PageStatus.Active
    readonly property bool canScan: viewFinder && viewFinder.source.cameraState === Camera.ActiveState

    signal scanCompleted(var token)

    onCanShowViewFinderChanged: {
        if (canShowViewFinder) {
            viewFinder = viewFinderComponent.createObject(viewFinderContainer, {
                viewfinderResolution: viewFinderContainer.viewfinderResolution,
                digitalZoom: FoilAuthSettings.scanZoom,
                orientation: orientationAngle()
            })

            if (viewFinder.source.availability === Camera.Available) {
                console.log("created viewfinder")
                viewFinder.source.start()
            } else {
                console.log("oops, couldn't create viewfinder...")
            }
        } else {
            viewFinder.source.stop()
            viewFinder.destroy()
            viewFinder = null
        }
    }

    onCanScanChanged: {
        if (canScan) {
            scanner.start()
        } else {
            scanner.stop()
        }
    }

    function orientationAngle() {
        switch (orientation) {
        case Orientation.Landscape: return 90
        case Orientation.PortraitInverted: return 180
        case Orientation.LandscapeInverted: return 270
        case Orientation.Portrait: default: return  0
        }
    }

    onOrientationChanged: {
        if (viewFinder) {
            viewFinder.orientation = orientationAngle()
        }
    }

    Component {
        id: hintComponent
        Hint { }
    }

    function showHint(text) {
        if (!hint) {
            hint = hintComponent.createObject(page)
        }
        hint.text = text
        hint.opacity = 1.0
    }

    function hideHint() {
        if (hint) {
            hint.opacity = 0.0
        }
    }

    QrCodeScanner {
        id: scanner

        property string lastInvalidCode
        viewFinderItem: viewFinderContainer
        rotation: orientationAngle()

        onScanFinished: {
            if (result.valid) {
                var token = FoilAuth.parseUri(result.text)
                if (token.valid) {
                    markImageProvider.image = image
                    markImage.visible = true
                    unsupportedCodeNotification.close()
                    scanCompleted(token)
                    pageStackPopTimer.start()
                } else if (lastInvalidCode !== result.text) {
                    lastInvalidCode = result.text
                    markImageProvider.image = image
                    markImage.visible = true
                    unsupportedCodeNotification.publish()
                    restartScanTimer.start()
                } else {
                    if (page.canScan) {
                        scanner.start()
                    }
                }
            } else if (page.canScan) {
                scanner.start()
            }
        }
    }

    Timer {
        id: pageStackPopTimer

        interval:  2000
        onTriggered: pageStack.pop()
    }

    Timer {
        id: restartScanTimer

        interval:  1000
        onTriggered: {
            markImage.visible = false
            markImageProvider.clear()
            if (page.canScan) {
                scanner.start()
            }
        }
    }

    Notification {
        id: unsupportedCodeNotification

        //: Warning notification
        //% "Invalid or unsupported QR code"
        previewBody: qsTrId("foilauth-notification-unsupported_qrcode")
        expireTimeout: 2000
        Component.onCompleted: {
            if ("icon" in unsupportedCodeNotification) {
                unsupportedCodeNotification.icon = "icon-s-high-importance"
            }
        }
    }

    Component {
        id: viewFinderComponent

        ViewFinder {
        }
    }

    Column {
        spacing: Theme.paddingLarge
        anchors.centerIn: parent
        width: parent.width

        onXChanged: viewFinderContainer.updateViewFinderPosition()
        onYChanged: viewFinderContainer.updateViewFinderPosition()

        Rectangle {
            id: viewFinderContainer

            readonly property bool canSwitchResolutions: typeof ViewfinderResolution_4_3 !== "undefined" &&
                typeof ViewfinderResolution_16_9 !== "undefined"
            readonly property int defaultShortSide: Math.floor((Math.min(Screen.width, Screen.height) - Theme.itemSizeLarge - 2 * Theme.paddingLarge)) & (-16)
            readonly property int defaultLongSide: Math.floor((Math.max(Screen.width, Screen.height) * 0.6)) & (-16)
            readonly property int defaultWidth: page.isPortrait ? defaultShortSide : defaultLongSide
            readonly property int narrowWidth: Math.floor(page.isPortrait ? height/16*9 : height*16/9) & (-2)
            readonly property int wideWidth: Math.floor(page.isPortrait ? height/4*3 : height*4/3) & (-2)
            readonly property size viewfinderResolution: canSwitchResolutions ?
                (FoilAuthSettings.scanWideMode ? ViewfinderResolution_4_3 : ViewfinderResolution_16_9) :
                Qt.size(0,0)
            anchors.horizontalCenter: parent.horizontalCenter
            width: canSwitchResolutions ? (FoilAuthSettings.scanWideMode ? wideWidth : narrowWidth) : defaultWidth
            height: page.isPortrait ? defaultLongSide : defaultShortSide
            color: "#20000000"

            onWidthChanged: updateViewFinderPosition()
            onHeightChanged: updateViewFinderPosition()
            onXChanged: updateViewFinderPosition()
            onYChanged: updateViewFinderPosition()

            onViewfinderResolutionChanged: {
                if (viewFinder && viewfinderResolution && canSwitchResolutions) {
                    viewFinder.viewfinderResolution = viewfinderResolution
                }
            }

            function updateViewFinderPosition() {
                scanner.viewFinderRect = Qt.rect(x + parent.x, y + parent.y, width, height)
            }
        }

        Item {
            height: Math.max(flashButton.height, zoomSlider.height, aspectButton.height)
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width

            Item {
                height: parent.height
                anchors {
                    left: parent.left
                    right: zoomSlider.left
                }
                visible: TorchSupported
                HintIconButton {
                    id: flashButton

                    anchors.centerIn: parent
                    icon.source: viewFinder && viewFinder.flashOn ?
                            "image://theme/icon-camera-flash-on" :
                            "image://theme/icon-camera-flash-off"
                    //: Hint label
                    //% "Toggle flashlight"
                    hint: qsTrId("foilauth-scan-hint_toggle_flash")
                    onShowHint: page.showHint(hint)
                    onHideHint: page.hideHint()
                    onClicked: if (viewFinder) viewFinder.toggleFlash()
                }
            }

            Slider {
                id: zoomSlider

                //: Slider label
                //% "Zoom"
                label: qsTrId("foilauth-scan-zoom_label")
                anchors.horizontalCenter: parent.horizontalCenter
                width: viewFinderContainer.narrowWidth
                leftMargin: 0
                rightMargin: 0
                minimumValue: 1.0
                maximumValue: 70.0
                value: 1
                stepSize: 5
                onValueChanged: {
                    FoilAuthSettings.scanZoom = value
                    if (viewFinder) {
                        viewFinder.digitalZoom = value
                    }
                }
                Component.onCompleted: {
                    value = FoilAuthSettings.scanZoom
                    if (viewFinder) {
                        viewFinder.digitalZoom = value
                    }
                }
            }

            Item {
                height: parent.height
                visible: viewFinderContainer.canSwitchResolutions
                anchors {
                    left: zoomSlider.right
                    right: parent.right
                }
                HintIconButton {
                    id: aspectButton

                    readonly property string icon_16_9: iconSourcePrefix + Qt.resolvedUrl("images/resolution_16_9.svg")
                    readonly property string icon_4_3: iconSourcePrefix +  Qt.resolvedUrl("images/resolution_4_3.svg")
                    anchors.centerIn: parent
                    icon {
                        source: FoilAuthSettings.scanWideMode ? icon_4_3 : icon_16_9
                        sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
                    }
                    //: Hint label
                    //% "Switch the aspect ratio between 9:16 and 3:4"
                    hint: qsTrId("foilauth-scan-hint_aspect_ratio")
                    onShowHint: page.showHint(hint)
                    onHideHint: page.hideHint()
                    onClicked: FoilAuthSettings.scanWideMode = !FoilAuthSettings.scanWideMode
                }
            }
        }
    }

    Image {
        id: markImage

        z: 2
        x: scanner.viewFinderRect.x
        y: scanner.viewFinderRect.y
        width: scanner.viewFinderRect.width
        height: scanner.viewFinderRect.height
        visible: false
        source: markImageProvider.source
        fillMode: Image.PreserveAspectCrop
    }

    HarbourSingleImageProvider {
        id: markImageProvider
    }
}
