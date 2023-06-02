import QtQuick
import "." as Cdt

Loader {
  id: root
  property bool isProjectOpened: false
  property var model
  Component {
    id: nativeMenuBar
    Cdt.NativeMenuBar {
      isProjectOpened: root.isProjectOpened
      model: root.model
    }
  }
  Component {
    id: crossPlatformMenuBar
    Cdt.CrossPlatformMenuBar {
      isProjectOpened: root.isProjectOpened
      model: root.model
    }
  }
  sourceComponent: Qt.platform.os === "osx" ? nativeMenuBar : crossPlatformMenuBar
}
