/*
 * Copyright 2013  Heena Mahour <heena393@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.0
import org.kde.plasma.calendar 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.plasma.extras 2.0 as PlasmaExtras

Grid {
    id : calendarDays
    columns : monthCalendar.days 
    rows : 1 + monthCalendar.weeks
    spacing : 0
    property Item selectedItem

    Repeater {
        id : days
        model : monthCalendar.days
        Item {
            width : cellWidth
            height : cellHeight
            Components.Label {
                text : Qt.formatDate(new Date(showDate.getFullYear(), showDate.getMonth(), index - firstDay +1), "ddd");
                horizontalAlignment : Text.AlignHCenter
                anchors.centerIn : parent
            }
        }
    }

    Repeater {
        id : repeater
        model : monthCalendar.daysModel
        Rectangle {
            id : myRectangle
            width : cellWidth
            height : cellHeight
            color : (dateMouse.containsMouse) ? "#eeeeee" : "transparent"
            border.color : isToday(dayNumber+"/"+monthNumber+"/"+yearNumber) ? "blue" : calendarDays.selectedItem == myRectangle ? "black" : "transparent"
            Components.Label {
                id : label
                anchors.centerIn : parent
                text : dayNumber
                font.bold : (containsEventItems)||(containsTodoItems) ? true : false
                opacity : (isPreviousMonth || isNextMonth || dateMouse.containsMouse) ? 0.5 : 1.0
            }
            MouseArea {
                id : dateMouse
                anchors.fill : parent
                hoverEnabled : true
                onClicked : {
                    var rowNumber = Math.floor ( index / 7)   ;
                    week = 1+monthCalendar.weeksModel[rowNumber];
                    date = dayNumber + "/" + monthNumber + "/" + yearNumber
                    calendarDays.selectedItem = myRectangle
                }
            }
        }
    }
}
