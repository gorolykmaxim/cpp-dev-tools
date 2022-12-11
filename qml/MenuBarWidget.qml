import QtQuick

Loader {
  Component {
    id: nativeWidget
    NativeMenuBarWidget {
      model: hActions
    }
  }
  Component {
    id: crossPlatformWidget
    CrossPlatformMenuBarWidget {
      model: hActions
    }
  }
  sourceComponent: useNativeMenuBar ? nativeWidget : crossPlatformWidget
}
