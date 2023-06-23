import QtQuick as QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

QtQuick.FocusScope {
  property alias model: tableView.model
  enabled: !tableView.model.placeholderText
  QtQuick.Connections {
    target: tableView.model
    function onCurrentChanged(i) {
      const cell = tableView.cellAtIndex(i);
      tableView.positionViewAtCell(cell, QtQuick.TableView.Contain, Qt.point(0, 0));
    }
  }
  Cdt.PlaceholderText {
    id: placeholder
    anchors.fill: parent
    visible: tableView.model.placeholderText
    text: tableView.model.placeholderText
    color: tableView.model.placeholderColor
  }
  ColumnLayout {
    spacing: 0
    anchors.fill: parent
    visible: !tableView.model.placeholderText
    HorizontalHeaderView {
      id: horizontalHeaderView
      syncView: tableView
      Layout.fillWidth: true
      clip: true
      delegate: Cdt.Text {
        text: display
        padding: Theme.basePadding
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        background: QtQuick.Rectangle {
          anchors.fill: parent
          color: Theme.colorBackground
          border.width: 1
          border.color: Theme.colorBorder
        }
      }
    }
    QtQuick.TableView {
      id: tableView
      clip: true
      focus: true
      Layout.fillWidth: true
      Layout.fillHeight: true
      boundsBehavior: QtQuick.Flickable.StopAtBounds
      columnWidthProvider: function(col) {
        const w = explicitColumnWidth(col);
        return w < 0 ? Math.max(tableView.width / tableView.columns, 150) : w;
      }
      delegate: Cdt.Text {
        required property int row
        text: display
        elide: Text.ElideRight
        padding: Theme.basePadding
        maximumLineCount: 1
        background: QtQuick.Rectangle {
          anchors.fill: parent
          color: row % 2 > 0 ? Theme.colorBackground : "#323231"
          border.width: 1
          border.color: current && tableView.activeFocus ? Theme.colorPrimary : "transparent"
        }
        QtQuick.MouseArea {
          anchors.fill: parent
          acceptedButtons: Qt.LeftButton | Qt.RightButton
          onPressed: function (e) {
            tableView.forceActiveFocus();
            tableView.model.setCurrent(row, column);
            if (e.button === Qt.RightButton) {
              contextMenu.open();
            }
          }
        }
      }
      QtQuick.Keys.onPressed: function (e) {
        e.accepted = !(e.modifiers & Qt.ControlModifier) && model.moveCurrent(e.key, bottomRow - topRow);
      }
      ScrollBar.vertical: ScrollBar {}
      ScrollBar.horizontal: ScrollBar {}
    }
    Menu {
      id: contextMenu
      MenuItem {
        text: "Copy Cell Value"
        enabled: tableView.activeFocus
        shortcut: "Ctrl+C"
        onTriggered: tableView.model.copyCurrentValue()
      }
      MenuItem {
        text: "Inspect Cell Value"
        enabled: tableView.activeFocus
        shortcut: "Ctrl+I"
        onTriggered: tableView.model.displayCurrentValueInDialog()
      }
    }
  }
}
