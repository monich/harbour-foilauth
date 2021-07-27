import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property var tokens: []

    signal tokenSelected(var token)

    SilicaListView {
        anchors.fill: parent
        width: parent.width
        clip: true
        model: tokens

        header: Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            height: implicitHeight + 2 * Theme.paddingLarge
            font.pixelSize: Theme.fontSizeExtraLarge
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            verticalAlignment: Text.AlignVCenter
            //: Wrappable page title
            //% "This QR code contains multiple tokens. Please select one:"
            text: qsTrId("foilauth-select_token-title")
        }

        delegate: BackgroundItem {
            id: delegate

            height: Theme.itemSizeMedium

            readonly property var token: modelData

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    text: token.label
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                }

                Label {
                    text: token.issuer
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    visible: text !== ""
                }
            }

            onClicked: page.tokenSelected(token)
        }

        VerticalScrollDecorator { }
    }
}
