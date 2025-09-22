import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0

Page {
    property alias title: pageHeader.title

    // jolla-settings expects these properties:
    property var applicationName
    property var applicationIcon

    readonly property string _rootPath: "/apps/harbour-foilauth/"

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content

            width: parent.width

            PageHeader {
                id: pageHeader

                rightMargin: Theme.horizontalPageMargin + (appIcon.visible ? (height - appIcon.padding) : 0)
                title: applicationName ? applicationName : "Foil Auth"
                description: applicationName ?
                    //: Settings page header description (app version)
                    //% "Version %1"
                    qsTrId("foilauth-settings_page-header-version").arg("1.1.11") : ""

                Image {
                    id: appIcon
                    readonly property int padding: Theme.paddingLarge
                    readonly property int size: pageHeader.height - 2 * padding
                    x: pageHeader.width - width - Theme.horizontalPageMargin
                    y: padding
                    width: size
                    height: size
                    sourceSize: Qt.size(size,size)
                    source: applicationIcon ? applicationIcon : ""
                    visible: appIcon.status === Image.Ready
                }
            }

            TextSwitch {
                //: Text switch label
                //% "Use the front camera"
                text: qsTrId("foilauth-settings_page-front_camera-text")
                //: Text switch description
                //% "You can use the front camera for scanning QR codes in case if your back camera is scratched."
                description: qsTrId("foilauth-settings_page-front_camera-description")
                automaticCheck: false
                checked: frontCameraConfig.value
                onClicked: frontCameraConfig.value = !frontCameraConfig.value

                ConfigurationValue {
                    id: frontCameraConfig

                    key: _rootPath + "frontCamera"
                    defaultValue: false
                }
            }

            TextSwitch {
                //: Text switch label
                //% "Automatic locking"
                text: qsTrId("foilauth-settings_page-autolock-text")
                //: Text switch description
                //% "Require to enter Foil password after unlocking the screen."
                description: qsTrId("foilauth-settings_page-autolock-description")
                automaticCheck: false
                checked: autoLockConfig.value
                onClicked: autoLockConfig.value = !autoLockConfig.value

                ConfigurationValue {
                    id: autoLockConfig

                    key: _rootPath + "autoLock"
                    defaultValue: true
                }
            }

            Slider {
                readonly property int min: value /60
                readonly property int sec: value  % 60

                visible: opacity > 0
                opacity: autoLockConfig.value ? 1.0 : 0.0
                width: parent.width
                minimumValue: 0
                maximumValue: 300
                value: autoLockTimeConfig.value / 1000
                stepSize: 5
                //: Slider label
                //% "Locking delay"
                label: qsTrId("foilauth-settings_page-autolock_delay-label")
                valueText: !value ?
                    //: Slider value (no delay)
                    //% "No delay"
                    qsTrId("foilauth-settings_page-autolock_delay-value-no_delay") :
                    //: Slider value
                    //% "%1 sec"
                    !min ? qsTrId("foilauth-settings_page-autolock_delay-value-sec",value).arg(sec) :
                    //: Slider value
                    //% "%1 min"
                    !sec ? qsTrId("foilauth-settings_page-autolock_delay-value-min",value).arg(min) :
                    qsTrId("foilauth-settings_page-autolock_delay-value-min",value).arg(min) + " " +
                    qsTrId("foilauth-settings_page-autolock_delay-value-sec",value).arg(sec)
                onSliderValueChanged: autoLockTimeConfig.value = value * 1000
                FadeAnimation on opacity { }

                ConfigurationValue {
                    id: autoLockTimeConfig

                    key: _rootPath + "autoLockTime"
                    defaultValue: 15000
                }
            }
        }
    }
}
