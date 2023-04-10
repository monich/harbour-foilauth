import QtQuick 2.0
import QtMultimedia 5.4
import Sailfish.Silica 1.0
import Sailfish.Media 1.0
import org.nemomobile.policy 1.0
import org.nemomobile.notifications 1.0
import harbour.foilauth 1.0

import "harbour"

Page {
    id: thisPage

    property Item _viewFinder
    property Item _hint

    readonly property bool _canUseVolumeKeys: FoilAuthSettings.volumeZoom && Qt.application.active &&
        (status === PageStatus.Active)
    readonly property bool canScan: _viewFinder && _viewFinder.source.cameraState === Camera.ActiveState
    readonly property bool canShowViewFinder: Qt.application.active &&
        (status === PageStatus.Active || status === PageStatus.Deactivating)

    signal tokenDetected(var token)
    signal tokensDetected(var tokens)
    signal skip()

    onStatusChanged: {
        if (status === PageStatus.Active) {
            markImage.visible = false
        }
    }

    onCanShowViewFinderChanged: {
        if (canShowViewFinder) {
            _viewFinder = viewFinderComponent.createObject(viewFinderContainer, {
                viewfinderResolution: viewFinderContainer.viewfinderResolution,
                digitalZoom: FoilAuthSettings.scanZoom,
                orientation: orientationAngle()
            })

            if (_viewFinder.source.availability === Camera.Available) {
                console.log("created viewfinder")
                _viewFinder.source.start()
            } else {
                console.log("oops, couldn't create viewfinder...")
            }
        } else {
            _viewFinder.source.stop()
            _viewFinder.destroy()
            _viewFinder = null
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
        if (_viewFinder) {
            _viewFinder.orientation = orientationAngle()
        }
    }

    Component {
        id: hintComponent
        Hint { }
    }

    function showHint(text) {
        if (!_hint) {
            _hint = hintComponent.createObject(thisPage)
        }
        _hint.text = text
        _hint.opacity = 1.0
    }

    function hideHint() {
        if (_hint) {
            _hint.opacity = 0.0
        }
    }

    FoilAuthImportModel {
        id: importModel
    }

    QrCodeScanner {
        id: scanner

        property string lastInvalidCode
        viewFinderItem: viewFinderContainer
        rotation: orientationAngle()

        onScanFinished: {
            importModel.setUri(result.text)
            if (importModel.count > 0) {
                markImageProvider.image = image
                markImage.visible = true
                unsupportedCodeNotification.close()
                pageStackPopTimer.start()
            } else if (lastInvalidCode !== result.text) {
                lastInvalidCode = result.text
                markImageProvider.image = image
                markImage.visible = true
                unsupportedCodeNotification.publish()
                restartScanTimer.start()
            } else if (thisPage.canScan) {
                scanner.start()
            }
        }
    }

    Timer {
        id: pageStackPopTimer

        interval: 1000
        onTriggered: {
            var n = importModel.count
            if (n === 1) {
                thisPage.tokenDetected(importModel.getToken(0))
            } else if (n > 1) {
                thisPage.tokensDetected(importModel)
            }
        }
    }

    Timer {
        id: restartScanTimer

        interval:  1000
        onTriggered: {
            markImage.visible = false
            markImageProvider.clear()
            if (thisPage.canScan) {
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
            onMaximumDigitalZoom: FoilAuthSettings.maxZoom = value
        }
    }

    HarbourFitLabel {
        id: titleLabel

        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        height: isPortrait ? Theme.itemSizeLarge : Theme.itemSizeSmall
        maxFontSize: isPortrait ? Theme.fontSizeExtraLarge : Theme.fontSizeLarge
        //: Page title (suggestion to scan QR code)
        //% "Scan QR code"
        text: qsTrId("foilauth-scan-title")
    }

    Item {
        anchors {
            top: titleLabel.bottom
            topMargin: Theme.paddingMedium
            bottom: toolBar.top
            bottomMargin: Theme.paddingLarge
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
        }

        onXChanged: viewFinderContainer.updateViewFinderPosition()
        onYChanged: viewFinderContainer.updateViewFinderPosition()
        onWidthChanged: viewFinderContainer.updateViewFinderPosition()
        onHeightChanged: viewFinderContainer.updateViewFinderPosition()

        Rectangle {
            id: viewFinderContainer

            readonly property bool canSwitchResolutions:
                FoilAuthSettings.wideCameraResolution.width > 0 && FoilAuthSettings.wideCameraResolution.height > 0 &&
                FoilAuthSettings.narrowCameraResolution.width > 0 && FoilAuthSettings.narrowCameraResolution.height > 0
            readonly property size viewfinderResolution: canSwitchResolutions ?
                (FoilAuthSettings.scanWideMode ? FoilAuthSettings.wideCameraResolution : FoilAuthSettings.narrowCameraResolution) :
                Qt.size(0,0)
            readonly property real ratio: canSwitchResolutions ?
                (FoilAuthSettings.scanWideMode ? FoilAuthSettings.wideCameraRatio : FoilAuthSettings.narrowCameraRatio) :
                (FoilAuthSettings.wideCameraResolution.width > 0 && FoilAuthSettings.wideCameraResolution.height > 0) ?
                FoilAuthSettings.wideCameraRatio : FoilAuthSettings.narrowCameraRatio

            readonly property int portraitWidth: Math.floor((parent.height/parent.width > ratio) ? parent.width : parent.height/ratio)
            readonly property int portraitHeight: Math.floor((parent.height/parent.width > ratio) ? (parent.width * ratio) : parent.height)
            readonly property int landscapeWidth: Math.floor((parent.width/parent.height > ratio) ? (parent.height * ratio) : parent.width)
            readonly property int landscapeHeight: Math.floor((parent.width/parent.height > ratio) ? parent.height : (parent.width / ratio))

            anchors.centerIn: parent
            width: thisPage.isPortrait ? portraitWidth : landscapeWidth
            height: thisPage.isPortrait ? portraitHeight : landscapeHeight
            color: "#20000000"

            onWidthChanged: updateViewFinderPosition()
            onHeightChanged: updateViewFinderPosition()
            onXChanged: updateViewFinderPosition()
            onYChanged: updateViewFinderPosition()

            onViewfinderResolutionChanged: {
                if (_viewFinder && viewfinderResolution && canSwitchResolutions) {
                    _viewFinder.viewfinderResolution = viewfinderResolution
                }
            }

            function updateViewFinderPosition() {
                scanner.viewFinderRect = Qt.rect(x + parent.x, y + parent.y, _viewFinder ? _viewFinder.width : width, _viewFinder ? _viewFinder.height : height)
            }

            function updateSupportedResolution_4_3(res) {
                if (res.width > FoilAuthSettings.wideCameraResolution.width) {
                    FoilAuthSettings.wideCameraResolution = res
                }
            }

            function updateSupportedResolution_16_9(res) {
                if (res.width > FoilAuthSettings.narrowCameraResolution.width) {
                    FoilAuthSettings.narrowCameraResolution = res
                }
            }

            Connections {
                target: _viewFinder
                onSupportedWideResolutionChanged: viewFinderContainer.updateSupportedResolution_4_3(_viewFinder.supportedWideResolution)
                onSupportedNarrowResolutionChanged: viewFinderContainer.updateSupportedResolution_16_9(_viewFinder.supportedNarrowResolution)
            }
        }
    }

    Item {
        id: toolBar

        height: Math.max(flashButton.height, zoomSlider.height, aspectButton.height)
        width: parent.width

        anchors {
            left: parent.left
            bottomMargin: Theme.paddingLarge
        }

        HarbourHintIconButton {
            id: flashButton

            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }

            visible: TorchSupported
            icon.source: (_viewFinder && _viewFinder.flashOn) ? "images/flash-on.svg" : "images/flash-off.svg"
            //: Hint label
            //% "Toggle flashlight"
            hint: qsTrId("foilauth-scan-hint_toggle_flash")
            onShowHint: thisPage.showHint(_hint)
            onHideHint: thisPage.hideHint()
            onClicked: if (_viewFinder) _viewFinder.toggleFlash()
        }

        Slider {
            id: zoomSlider

            anchors {
                left: parent.left
                leftMargin: 2 * Theme.horizontalPageMargin + Theme.itemSizeSmall
                right: parent.right
                rightMargin: 2 * Theme.horizontalPageMargin + Theme.itemSizeSmall
            }

            //: Slider label
            //% "Zoom"
            label: qsTrId("foilauth-scan-zoom_label")
            leftMargin: 0
            rightMargin: 0
            minimumValue: 1.0
            maximumValue: FoilAuthSettings.maxZoom
            value: 1.0
            stepSize: (maximumValue - minimumValue)/100

            onValueChanged: {
                FoilAuthSettings.scanZoom = value
                if (_viewFinder) {
                    _viewFinder.digitalZoom = value
                }
            }

            Component.onCompleted: {
                value = FoilAuthSettings.scanZoom
                if (_viewFinder) {
                    _viewFinder.digitalZoom = value
                }
            }

            function zoomIn() {
                if (value < maximumValue) {
                    var newValue = value + stepSize
                    if (newValue < maximumValue) {
                        value = newValue
                    } else {
                        value = maximumValue
                    }
                }
            }

            function zoomOut() {
                if (value > minimumValue) {
                    var newValue = value - stepSize
                    if (newValue > minimumValue) {
                        value = newValue
                    } else {
                        value = minimumValue
                    }
                }
            }
        }

        HarbourHintIconButton {
            id: aspectButton

            anchors {
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            visible: viewFinderContainer.canSwitchResolutions
            icon.source: FoilAuthSettings.scanWideMode ? "images/resolution_4_3.svg" : "images/resolution_16_9.svg"
            //: Hint label
            //% "Switch the aspect ratio between 9:16 and 3:4"
            hint: qsTrId("foilauth-scan-hint_aspect_ratio")
            onShowHint: thisPage.showHint(_hint)
            onHideHint: thisPage.hideHint()
            onClicked: FoilAuthSettings.scanWideMode = !FoilAuthSettings.scanWideMode
        }
    }

    Button {
        id: skipButton

        anchors.margins: Theme.paddingLarge
        //: Button label (skip scanning)
        //% "Skip"
        text: qsTrId("foilauth-scan-skip_button")
        onClicked: thisPage.skip()
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

    MediaKey{
        enabled: _canUseVolumeKeys
        key: Qt.Key_VolumeUp
        onPressed: zoomSlider.zoomIn()
        onRepeat: zoomSlider.zoomIn()
    }

    MediaKey{
        enabled: _canUseVolumeKeys
        key: Qt.Key_VolumeDown
        onPressed: zoomSlider.zoomOut()
        onRepeat: zoomSlider.zoomOut()
    }

    Permissions{
        autoRelease: true
        applicationClass: "camera"
        enabled: _canUseVolumeKeys

        Resource{
            type: Resource.ScaleButton
            optional: true
        }
    }

    states: [
        State {
            name: "portrait"
            when: !isLandscape
            changes: [
                AnchorChanges {
                    target: toolBar
                    anchors {
                        right: parent.right
                        bottom: skipButton.top
                    }
                },
                AnchorChanges {
                    target: skipButton
                    anchors {
                        right: undefined
                        bottom: parent.bottom
                        horizontalCenter: parent.horizontalCenter
                        verticalCenter: undefined
                    }
                }
            ]
        },
        State {
            name: "landscape"
            when: isLandscape
            changes: [
                AnchorChanges {
                    target: toolBar
                    anchors {
                        right: skipButton.left
                        bottom: parent.bottom
                    }
                },
                AnchorChanges {
                    target: skipButton
                    anchors {
                        right: parent.right
                        bottom: undefined
                        horizontalCenter: undefined
                        verticalCenter: toolBar.verticalCenter
                    }
                }
            ]
        }
    ]
}
