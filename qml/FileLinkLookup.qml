import cdt

FileLinkLookupController {
  property var menuItems: [
    {
      separator: true,
    },
    {
      text: "Open File In Editor",
      shortcut: gSC("FileLinkLookup", "Open File In Editor"),
      callback: () => openCurrentFileLink()
    },
    {
      text: "Previous File Link",
      shortcut: gSC("FileLinkLookup", "Previous File Link"),
      callback: () => goToLink(false)
    },
    {
      text: "Next File Link",
      shortcut: gSC("FileLinkLookup", "Next File Link"),
      callback: () => goToLink(true)
    },
  ]
}
