import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Cdt.TextSearchBar {
  id: root
  property string text: ""
  property alias formatter: controller.formatter
  signal searchResultsChanged()
  signal selectResult(int offset, int length)
  signal replaceResults(list<int> offsets, list<int> lengths, string text)
  onTextChanged: {
    if (visible) {
      controller.search(searchTerm, text, root.activeFocus, false)
    }
  }
  onSearch: controller.search(searchTerm, root.text, true, true)
  onReplace: replaceAll => controller.replaceSearchResultWith(replacementTerm, replaceAll)
  onGoToSearchResult: next => controller.goToSearchResult(next)
  function displayAndGoToResult(offset, text) {
    display(text);
    controller.goToResultWithStartAt(offset);
  }
  searchResultCount: controller.searchResultsCount
  noSearchResults: controller.searchResultsEmpty
  TextSearchController {
    id: controller
    onSearchResultsChanged: root.searchResultsChanged()
    onReplaceResults: (o, l, t) => root.replaceResults(o, l, t)
    onSelectResult: (o, l) => root.selectResult(o, l)
  }
}
