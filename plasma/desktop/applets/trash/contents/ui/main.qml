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
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.qtextracomponents 2.0
import org.kde.dirmodel 2.0

MouseArea {
    id:root
    
    property int minimumWidth//:formFactor == Horizontal ? height : 1
    property int minimumHeight//:formFactor == Vertical ? width  : 1
    property int formFactor: plasmoid.formFactor
    property bool constrained: false//vertical is not referenced in plasma2 ..formFactor==Vertical||formFactor==Horizontal

    hoverEnabled : true
    onClicked : Qt.openUrlExternally("trash:/");
    
    DirModel {
        id : dirModel
        url : "trash:/"
    }
    
    Connections {
        target : plasmoid
        onFormFactorChanged : {
            root.formFactor = plasmoid.formFactor
            if(root.formFactor==Planar || root.formFactor == MediaCenter )
            {
               minimumWidth=root.width/3.5
               minimumHeight=root.height/3.5
            }
        }
    }

    function action_open() {
        Qt.openUrlExternally("trash:/");
    }

    function action_empty() {
        queryDialog.open();
    }
    
    Component.onCompleted: { 
        plasmoid.backgroundHints = 0;

        plasmoid.setAction("open", "Open","document-open");

        plasmoid.setAction("empty","Empty","trash-empty");
        plasmoid.popupIcon = QIcon("user-trash");
        plasmoid.aspectRatioMode = IgnoreAspectRatio;
    }
<<<<<<< HEAD
    
    MouseArea {
        id : mouseArea
        hoverEnabled : true
        onClicked : Qt.openUrlExternally("trash:/");
        anchors.fill : parent
        PlasmaCore.IconItem {
            id:icon
            source: (dirModel.count > 0) ? i18n( "user-trash-full" ) : i18n ("user-trash")
            anchors{
                left : parent.left
                right : parent.right
                top : parent.top
                bottom : constrained ? parent.bottom : text.top
            }
            active: mouseArea.containsMouse
        }
        Components.Label {
            id : text
            text : (dirModel.count==0) ? i18n("Trash\nEmpty") : (dirModel.count==1)? i18n("Trash\nOne item") : i18n("Trash\n"+ dirModel.count + "items")
            anchors {
                left : parent.left
                bottom : parent.bottom  
                right : parent.right
            }
            horizontalAlignment : Text.AlignHCenter
            opacity : constrained ? 0 : 1
        }
        PlasmaCore.ToolTip {
            target : mouseArea
            mainText : i18n("Trash")
            subText : (dirModel.count==0) ? i18n("Trash \n Empty") :(dirModel.count==1) ? i18np(("Trash\nOne item", "Trash\n %1 items", dirModel.count ))
            image : (dirModel.count > 0) ? "user-trash-full" : "user-trash"
=======


    PlasmaCore.IconItem {
        id:icon
        source: (dirModel.count > 0) ? i18n( "user-trash-full" ) : i18n ("user-trash")
        anchors{
            left : parent.left
            right : parent.right
            top : parent.top
            bottom : constrained ? parent.bottom : text.top
        }
        active: root.containsMouse
    }
    Components.Label {
        id : text
        text : (dirModel.count==0) ? i18n(" Trash\nEmpty") : (dirModel.count==1)? i18n(" Trash\nOne item") : i18n(" Trash\n"+ dirModel.count + "items")
        anchors {
            left : parent.left
            bottom : parent.bottom  
            right : parent.right
>>>>>>> d223fe414efb0cab4d648c29cf593a0a56b2ec7e
        }
        horizontalAlignment : Text.AlignHCenter
        opacity : constrained ? 0 : 1
    }
    PlasmaCore.ToolTip {
        target : root
        mainText : i18n("Trash")
        subText : (dirModel.count==0) ? i18n("Trash \n Empty") :(dirModel.count==1) ? i18n("Trash \n One item") : i18n("Trash \n " + dirModel.count + "items")
        image : (dirModel.count > 0) ? "user-trash-full" : "user-trash"
    }


    Components.QueryDialog {
        id : queryDialog
        titleIcon : "user-trash"
        titleText : i18n("Empty Trash")
        message : i18n("Do you really want to empty the trash ? All the items will be deleted.")
        acceptButtonText : i18n("Empty Trash")
        rejectButtonText : i18n("Cancel")
        onAccepted : plasmoid.runCommand("ktrash", ["--empty"]);
        onRejected : queryDialog.close();
        visualParent : root
        }
}