import QtQuick
import "." as Cdt

Loader {
  id: root
  property var project
  property var model
  Component {
    id: nativeMenuBar
    Cdt.NativeMenuBar {
      project: root.project
      model: root.model
    }
  }
  Component {
    id: crossPlatformMenuBar
    Cdt.CrossPlatformMenuBar {
      project: root.project
      model: root.model
    }
  }
  sourceComponent: isMacOS ? nativeMenuBar : crossPlatformMenuBar
}
