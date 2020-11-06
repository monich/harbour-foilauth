import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

Item {
    property Page mainPage
    property var foilModel
    property int shownCount: 0

    readonly property bool isLandscape: mainPage && mainPage.isLandscape
    readonly property bool unlocking: !foilModel || !foilModel.count || unlockingTimer.running
    readonly property int realCount: foilModel ? foilModel.count : 0

    onRealCountChanged: syncCounts()
    onUnlockingChanged: syncCounts()

    function syncCounts() {
        if (!unlocking && realCount > shownCount) {
            shownCount++;
            if (realCount > shownCount) {
                // They still don't match (could be at the beginning)
                mismatchTimer.start()
            }
        }
    }

    Label {
        width: parent.width - 2*Theme.horizontalPageMargin
        height: implicitHeight
        anchors {
            bottom: busyIndicator.top
            bottomMargin: Theme.paddingLarge
            horizontalCenter: parent.horizontalCenter
        }
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.highlightColor
        text: unlocking ?
            //: Progress view label
            //% "Unlocking..."
            qsTrId("foilauth-decrypting_view-unlocking") :
            //: Progress view label
            //% "Decrypting..."
            qsTrId("foilauth-decrypting_view-decrypting")
    }

    HarbourFitLabel {
        anchors.fill: busyIndicator
        text: (shownCount > 0) ? shownCount : ""
        opacity: (foilModel && foilModel.foilState === FoilAuthModel.FoilDecrypting && !unlocking) ? 1 : 0
    }

    BusyIndicator {
        id: busyIndicator

        y: Math.floor(((isLandscape ? Screen.width : Screen.height) - height) /2)
        anchors.horizontalCenter: parent.horizontalCenter
        size: BusyIndicatorSize.Large
        running: true
    }

    Timer {
        id: unlockingTimer

        interval: 500
        running: true
    }

    Timer {
        id: mismatchTimer

        interval: 0
        onTriggered: {
            if (shownCount < realCount) {
                shownCount++
                if (shownCount < realCount) {
                    start();
                }
            }
        }
    }
}
