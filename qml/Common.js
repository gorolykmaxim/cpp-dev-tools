function findMenuByTitle(menuBar, menuTitle) {
  for (let i = 0; i < menuBar.menus.length; i++) {
    const menuObj = menuBar.menus[i];
    if (menuObj.title === menuTitle) {
      return menuObj;
    }
  }
  return null;
}
