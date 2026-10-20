# Taskbar, window icons and main window without QtWinExtras (PR #40, M07)

1. With the main window open, upload a screenshot (tray menu or the Upload button): the taskbar button shows a progress bar that fills as the upload runs, then a green "yes" overlay icon on the button for about two seconds, after which the overlay and progress clear.
2. Start an area capture and make it fail or be refused (for example a file target in a folder you cannot write): the taskbar button shows the red "no" overlay for about two seconds.
3. Take a window-picker screenshot and move the cursor over a few windows: each window's icon shows in the picker, as before.
4. Take a screenshot with "include the cursor" on: the cursor appears in the image with its shape and transparency.
5. The main window has the normal window background (no transparent "glass" area) and looks acceptable.
6. Right-click the taskbar button: there is no Lightscreen jump list any more (expected).
