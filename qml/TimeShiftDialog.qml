import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    property int timeshift // In minutes

    readonly property real screenHeight: isPortrait ? Screen.height : Screen.width
    readonly property int _fullRound: 24 * 60
    property int _timebase

    DialogHeader { id: dialogHeader  }

    TimePicker {
        id: timePicker

        property int _lastHour
        readonly property int _timeshift: _timebase + 60 * hour + minute
        readonly property int _absTimeshift: Math.abs(_timeshift)

        y:  Math.max((screenHeight - height)/2, dialogHeader.y + dialogHeader.height + Theme.paddingLarge)
        anchors.horizontalCenter: parent.horizontalCenter

        onHourChanged: {
            if (hour < _lastHour - 6) {
                _timebase += _fullRound
            } else if (hour > _lastHour + 6) {
                _timebase -= _fullRound
            }
            _lastHour = hour
        }

        Label {
            anchors.centerIn: parent
            font {
                pixelSize: Theme.fontSizeExtraLarge
                family: Theme.fontFamilyHeading
            }
            text: (timePicker._timeshift < 0 ? "-" : timePicker._timeshift > 0 ? "+" : "") +
                  xx(Math.floor(timePicker._absTimeshift / 60)) + ":" +
                  xx(timePicker._absTimeshift % 60)

            function xx(n) {
                return (n >= 10) ? n : "0" + n
            }
        }
    }

    onOpened: {
        _timebase = Math.floor(timeshift / _fullRound) * _fullRound
        if (timeshift < 0) _timebase  -= _fullRound
        var delta = timeshift - _timebase
        timePicker.hour = timePicker._lastHour = Math.floor(delta / 60)
        timePicker.minute = delta % 60
    }

    onDone: {
        if (result == DialogResult.Accepted) {
            timeshift = timePicker._timeshift
        }
    }
}
