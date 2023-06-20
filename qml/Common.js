const MAX_INT = 2147483647;

function findMenuByTitle(menuBar, menuTitle) {
  for (let i = 0; i < menuBar.menus.length; i++) {
    const menuObj = menuBar.menus[i];
    if (menuObj.title === menuTitle) {
      return menuObj;
    }
  }
  return null;
}

function handleRightClick(component, menu, event) {
  if (!event || event.button === Qt.RightButton) {
    // Context menu requires the right click target to already have
    // active focus, because it enables/disabled its shortcuts based on it.
    component.forceActiveFocus();
    menu.open();
  }
}

function pageUpList(listView) {
  const result = listView.currentIndex !== 0;
  listView.currentIndex = Math.max(listView.currentIndex - 10, 0);
  return result;
}

function pageDownList(listView) {
  const result = listView.currentIndex !== listView.count - 1;
  listView.currentIndex = Math.min(listView.currentIndex + 10, listView.count - 1);
  return result;
}

function handleListViewNavigation(event, listView) {
  if (event.matches(StandardKey.MoveToStartOfDocument)) {
    event.accepted = listView.currentIndex !== 0;
    listView.currentIndex = 0;
  } else if (event.matches(StandardKey.MoveToEndOfDocument)) {
    event.accepted = listView.currentIndex !== listView.count - 1;
    listView.currentIndex = listView.count - 1;
  } else if (event.key === Qt.Key_PageUp) {
    event.accepted = pageUpList(listView);
  } else if (event.key === Qt.Key_PageDown) {
    event.accepted = pageDownList(listView);
  }
}
