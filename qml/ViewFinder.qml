import QtQuick 2.0
import QtMultimedia 5.4
import Sailfish.Silica 1.0

VideoOutput {
    id: viewFinder

    layer.enabled: true
    anchors.fill: parent
    fillMode: VideoOutput.Stretch

    property alias beepSource: beep.source
    property bool playingBeep: false
    property size viewfinderResolution
    property bool completed
    property real digitalZoom: 1.0

    readonly property bool cameraActive: camera.cameraState === Camera.ActiveState
    readonly property bool tapFocusActive: focusTimer.running
    readonly property bool flashOn: camera.flash.mode !== Camera.FlashOff
    // Not sure why not just camera.orientation but this makes the camera
    // behave similar to what it does for Jolla Camera
    readonly property int cameraOrientation: 360 - camera.orientation

    // Camera doesn't know its maximumDigitalZoom until cameraStatus becomes
    // Camera.ActiveStatus and doesn't emit maximumDigitalZoomChanged signal
    signal maximumDigitalZoom(var value)

    onOrientationChanged: {
        if (camera.cameraState !== Camera.UnloadedState) {
            reloadTimer.restart()
        }
    }

    onViewfinderResolutionChanged: {
        if (viewfinderResolution && camera.viewfinder.resolution !== viewfinderResolution) {
            if (camera.cameraState === Camera.UnloadedState) {
                camera.viewfinder.resolution = viewfinderResolution
            } else {
                reloadTimer.restart()
            }
        }
    }

    onDigitalZoomChanged: {
        if (camera.cameraStatus === Camera.ActiveStatus) {
            camera.digitalZoom = digitalZoom
        }
    }

    Component.onCompleted: {
        if (viewfinderResolution) {
            camera.viewfinder.resolution = viewfinderResolution
        }
        completed = true
    }

    function turnFlashOn() {
        camera.flash.mode = Camera.FlashTorch
    }

    function turnFlashOff() {
        camera.flash.mode = Camera.FlashOff
    }

    function toggleFlash() {
        if (flashOn) {
            turnFlashOff()
        } else {
            turnFlashOn()
        }
    }

    function playBeep() {
        // Camera locks the output playback resource, we need
        // to stop it before we can play the beep. Luckily,
        // the viewfinder is typically covered with the marker
        // image so the user won't even notice the pause.
        playingBeep = true
        // The beep starts playing when the camera stops and
        // audio becomes available. When playback is stopped,
        // (playback state becomes Audio.StoppedState) the
        // camera is restarted.
        camera.stop()
    }

    function viewfinderToFramePoint(vx, vy) {
        var x = (vx - viewFinder.contentRect.x)/viewFinder.contentRect.width
        var y = (vy - viewFinder.contentRect.y)/viewFinder.contentRect.height
        switch (cameraOrientation) {
        case 90:  return Qt.point(y, 1 - x)
        case 180: return Qt.point(1 - x, 1 - y)
        case 270: return Qt.point(1 - y, x)
        default:  return Qt.point(x, y)
        }
    }

    function frameToViewfinderRect(rect) {
        var vx, vy, vw, vh
        switch (cameraOrientation) {
        case 90:
        case 270:
            vw = rect.height
            vh = rect.width
            break
        default:
            vw = rect.width
            vh = rect.height
            break
        }
        switch (cameraOrientation) {
        case 90:
            vx = 1 - rect.y - vw
            vy = rect.x
            break
        case 180:
            vx = 1 - rect.x - vw
            vy = 1 - rect.y - vh
            break
        case 270:
            vx = rect.y
            vy = 1 - rect.x - vh
            break
        default:
            vx = rect.x
            vy = rect.y
            break
        }
        return Qt.rect(viewFinder.contentRect.width * vx +  viewFinder.contentRect.x,
                       viewFinder.contentRect.height * vy + viewFinder.contentRect.y,
                       viewFinder.contentRect.width * vw, viewFinder.contentRect.height * vh)
    }

    source: Camera {
        id: camera

        flash.mode: Camera.FlashOff
        captureMode: Camera.CaptureVideo
        imageProcessing.whiteBalanceMode: flashOn ?
            CameraImageProcessing.WhiteBalanceFlash :
            CameraImageProcessing.WhiteBalanceTungsten
        cameraState: (completed && !reloadTimer.running) ?
            Camera.ActiveState : Camera.UnloadedState
        exposure {
            exposureCompensation: 1.0
            exposureMode: Camera.ExposureAuto
        }
        focus {
            focusMode: tapFocusActive ? Camera.FocusAuto : Camera.FocusContinuous
            focusPointMode: tapFocusActive ? Camera.FocusPointCustom : Camera.FocusPointAuto
        }
        onCameraStatusChanged: {
            if (cameraStatus === Camera.ActiveStatus) {
                // Camera doesn't emit maximumDigitalZoomChanged signal
                viewFinder.maximumDigitalZoom(maximumDigitalZoom)
                digitalZoom = viewFinder.digitalZoom
            }
        }
        onCameraStateChanged: {
            if (cameraState === Camera.ActiveState) {
                viewFinder.playingBeep = false
            } else if (cameraState === Camera.UnloadedState) {
                if (viewFinder.viewfinderResolution &&
                    viewFinder.viewfinderResolution !== viewfinder.resolution) {
                    viewfinder.resolution = viewFinder.viewfinderResolution
                }
            }
        }
    }

    Timer {
        id: reloadTimer
        interval: 100
    }

    // Display focus areas
    Repeater {
        model: camera.focus.focusZones
        delegate: Rectangle {
            visible: !scanner.grabbing &&
                     status !== Camera.FocusAreaUnused &&
                     camera.focus.focusPointMode === Camera.FocusPointCustom &&
                     camera.cameraStatus === Camera.ActiveStatus
            border {
                width: Math.round(Theme.pixelRatio * 2)
                color: status === Camera.FocusAreaFocused ? Theme.highlightColor : "white"
            }
            color: "transparent"

            readonly property rect mappedRect: frameToViewfinderRect(area)
            readonly property real diameter: Math.round(Math.min(mappedRect.width, mappedRect.height))

            x: Math.round(mappedRect.x + (mappedRect.width - diameter)/2)
            y: Math.round(mappedRect.y + (mappedRect.height - diameter)/2)
            width: diameter
            height: diameter
            radius: diameter / 2
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: {
            focusTimer.restart()
            camera.unlock()
            camera.focus.customFocusPoint = viewfinderToFramePoint(mouse.x, mouse.y)
            camera.searchAndLock()
        }
    }

    Timer {
        id: focusTimer

        interval: 5000
        onTriggered: camera.unlock()
    }

    Audio {
        id: beep
        volume: 1.0
        readonly property bool canPlay: source !== "" && availability == Audio.Available && playingBeep

        onCanPlayChanged: if (canPlay) play()

        onPlaybackStateChanged: {
            if (playbackState === Audio.StoppedState) {
                camera.start()
            }
        }
    }
}
