import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property alias mainPage: view.mainPage

    allowedOrientations: Orientation.All

    GenerateFoilKeyView {
        id: view

        anchors.fill: parent
        //: Label text
        //% "You are about to generate a new key"
        title: qsTrId("foilnotes-generate_key_page-title")
    }
}
