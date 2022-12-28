function getListModelValue(model, row, col) {
  return model.data(model.index(row, 0), col);
}

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
