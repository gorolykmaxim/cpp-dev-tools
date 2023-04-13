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
