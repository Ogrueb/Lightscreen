# Mixed-DPI capture after S04 (S04, bugs 4, 5, 11, 12, 23)

Needs two monitors at different scaling (for example 100% and 150% in Settings > System > Display), with the area action on a hotkey (or the tray menu's "Area"). Do steps 1-6 once with the 100% monitor as the main display and once with the other one, if you can. Compare sizes with any image viewer that shows pixel dimensions.

1. **Whole desktop.** Options > "Current monitor only" off, cursor capture on. Take a whole-screen screenshot: the file is the full native size of both monitors side by side (for example 1920 + 2560 wide), each monitor sharp and unscaled, nothing cut off or doubled; the cursor is drawn where it was.
2. **Current monitor.** Turn "Current monitor only" on. With the cursor on each monitor in turn, take a whole-screen screenshot: the file is that monitor at its native size (1920x1080, 2560x1440, ...), and the cursor is in the right place.
3. **Area on each monitor.** "Current monitor only" off. Open the area dialog: the desktop shown behind it lines up with the real desktop on both monitors (no jump, no zoom when it appears). Select a window's exact edges on the scaled monitor: the file shows that window with no strip of the background and nothing cut.
4. **Area across the boundary.** Select a rectangle that straddles the two monitors: the file contains exactly the marked pixels from both sides.
5. **Area, current monitor only.** Turn "Current monitor only" on and repeat step 3 on each monitor.
6. **Keyboard size at 150%.** In the area dialog on the 150% monitor, draw a rectangle, type `100x100`, Enter twice: the file is 150x150.
7. **Window capture** (bug 5). Take an active-window screenshot of a window on the secondary monitor, then one on the primary: neither has a border strip from the window behind it, and the window's own edge is not cut.
