/*
 *   Copyright 2011 Sebastian Kügler <sebas@kde.org>
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 1.1
import org.kde.plasma.core 0.1 as PlasmaCore

Item {
    id: batterymonitor
    property int minimumWidth: dialogItem.width
    property int minimumHeight: dialogItem.height

    property bool show_multiple_batteries: false
    property bool show_remaining_time: false

    Component.onCompleted: {
        plasmoid.addEventListener('ConfigChanged', configChanged);
    }

    function configChanged() {
        show_multiple_batteries = plasmoid.readConfig("showMultipleBatteries");
        show_remaining_time = plasmoid.readConfig("showRemainingTime");
    }

    property Component compactRepresentation: Component {
        MouseArea {
            id: mouseArea
            anchors.fill:parent
            onClicked: plasmoid.togglePopup()

            property QtObject pmSource: plasmoid.rootItem.pmSource

            Item {
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: width
                
                BatteryIcon {
                    monochrome: true
                    hasBattery: pmSource.data["Battery"]["Has Battery"]
                    percent: pmSource.data["Battery0"]["Percent"]
                    pluggedIn: pmSource.data["AC Adapter"]["Plugged in"]
                    anchors.fill: parent
                }
            }
        }
    }

    property QtObject pmSource: PlasmaCore.DataSource {
        id: pmSource
        engine: "powermanagement"
        connectedSources: ["AC Adapter", "Battery", "Battery0", "PowerDevil", "Sleep States"]
        interval: 0
    }

    PopupDialog {
        id: dialogItem
        percent: pmSource.data["Battery0"]["Percent"]
        hasBattery: pmSource.data["Battery"]["Has Battery"]
        pluggedIn: pmSource.data["AC Adapter"]["Plugged in"]
        screenBrightness: pmSource.data["PowerDevil"]["Screen Brightness"]
        remainingMsec: Number(pmSource.data["Battery"]["Remaining msec"])
        showRemainingTime: parent.show_remaining_time
        showSuspendButton: pmSource.data["Sleep States"]["Suspend"]
        showHibernateButton: pmSource.data["Sleep States"]["Hibernate"]
        onSuspendClicked: {
            plasmoid.togglePopup();
            service = pmSource.serviceForSource("PowerDevil");
            var operationName = callForType(type);
            operation = service.operationDescription(operationName);
            service.startOperationCall(operation);
        }
        onBrightnessChanged: {
            service = pmSource.serviceForSource("PowerDevil");
            operation = service.operationDescription("setBrightness");
            operation.brightness = screenBrightness;
            service.startOperationCall(operation);
        }
        property int cookie1: -1
        property int cookie2: -1
        onPowermanagementChanged: {
            service = pmSource.serviceForSource("PowerDevil");
            if (checked) {
                var op1 = service.operationDescription("stopSuppressingSleep");
                op1.cookie = cookie1;
                var op2 = service.operationDescription("stopSuppressingScreenPowerManagement");
                op2.cookie = cookie2;

                var job1 = service.startOperationCall(op1);
                job1.finished.connect(function(job) {
                    cookie1 = -1;
                });

                var job2 = service.startOperationCall(op2);
                job1.finished.connect(function(job) {
                    cookie2 = -1;
                });
            } else {
                var reason = i18n("The battery applet has enabled system-wide inhibition");
                var op1 = service.operationDescription("beginSuppressingSleep");
                op1.reason = reason;
                var op2 = service.operationDescription("beginSuppressingScreenPowerManagement");
                op2.reason = reason;

                var job1 = service.startOperationCall(op1);
                job1.finished.connect(function(job) {
                    cookie1 = job.result;
                });

                var job2 = service.startOperationCall(op2);
                job1.finished.connect(function(job) {
                    cookie2 = job.result;
                });
            }
        }

        function callForType(type) {
            if (type == ram) {
                return "suspendToRam";
            } else if (type == disk) {
                return "suspendToDisk";
            }

            return "suspendHybrid";
        }
    }
}