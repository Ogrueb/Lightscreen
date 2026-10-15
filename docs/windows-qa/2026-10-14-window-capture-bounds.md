# Window capture uses the visible window bounds (PR #18, bug 5)

Take a window capture of each case and open the file.

1. A non-maximized window next to another window: no stripe of the neighbouring window, and no edge of the window cut off.
2. A maximized window: the whole window, nothing cut.
3. A window partly off screen to the left or top: clean left and top edges, nothing cut on the right or bottom.
