import "." as Cdt
import cdt

Cdt.Pane {
  property alias text: text.text
  color: Theme.colorBgDark
  Cdt.Text {
    id: text
    anchors.fill: parent
    elide: Text.ElideRight
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
  }
}

