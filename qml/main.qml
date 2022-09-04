import QtQuick

Window {
  width: 1024
  height: 600
  title: "CPP Dev Tools"
  visible: true
  Loader {
    source: currentView
  }
}
