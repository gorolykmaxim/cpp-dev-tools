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
  ]
}
