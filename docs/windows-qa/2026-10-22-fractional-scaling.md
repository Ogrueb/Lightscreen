# Windows drawn at the exact display scale (Stage 7, scaling fix)

Set a monitor to 125%, then 150% (Settings > System > Display > Scale), signing out if Windows asks. On each:

1. Open the main window and Options: text and buttons are the same size as in other apps at that scale (until this fix they were drawn at 100% at 125%, and at 200% at 150%), and nothing is cut off or overlapping.
2. Take an area screenshot: the desktop behind the selector lines up with the real one (no zoom or offset), the frame and handles are crisp enough to aim with, and the saved file has the selected size in physical pixels (a typed `100x100` gives 125x125 or 150x150).
3. Take a whole-screen screenshot: the file has the monitor's full physical resolution.
4. With two monitors at different scales: drag the main window from one to the other; it resizes to the new scale and stays usable.
