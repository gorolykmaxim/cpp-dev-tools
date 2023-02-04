function findMenuByTitle(menuBar, menuTitle) {
  for (let i = 0; i < menuBar.menus.length; i++) {
    const menuObj = menuBar.menus[i];
    if (menuObj.title === menuTitle) {
      return menuObj;
    }
  }
  return null;
}

function onListAction(list, action, itemProp) {
  if (!list.currentItem) {
    return;
  }
  core.OnAction(action, [list.currentItem.itemModel[itemProp]]);
}

function callListActionOrOpenContextMenu(
    event, list, action, itemProp, contextMenu) {
  if (event.button == Qt.LeftButton) {
    onListAction(list, action, itemProp);
  } else if (event.button == Qt.RightButton) {
    contextMenu.open();
  }
}
