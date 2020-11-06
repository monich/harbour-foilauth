import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property Page mainPage
    property alias foilModel: view.foilModel

    allowedOrientations: Orientation.All

    // Otherwise width may change with a delay, causing visible layout changes
    onIsLandscapeChanged: width = isLandscape ? Screen.height : Screen.width

    GenerateFoilKeyView {
        id: view

        parentPage: page
        mainPage: page.mainPage
        anchors.fill: parent
        //: Label text
        //% "You are about to generate a new key"
        prompt: qsTrId("foilnotes-generate_key_page-title")
    }
}
