import cdt

FileLinkLookupController {
  property var menuItems: [
    {
      separator: true,
    },
    {
      text: "Open File In Editor",
      shortcut: "Ctrl+O",
      callback: () => openCurrentFileLink()
    },
    {
      text: "Previous File Link",
      shortcut: "Ctrl+Alt+Up",
      callback: () => goToLink(false)
    },
    {
      text: "Next File Link",
      shortcut: "Ctrl+Alt+Down",
      callback: () => goToLink(true)
    },
  ]
}
