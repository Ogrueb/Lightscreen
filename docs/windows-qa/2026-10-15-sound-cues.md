# Sound cues without QtMultimedia (PR #NN, M06, bug 3)

1. In Options, tick "Play a sound", save, and take a whole-screen screenshot: the Lightscreen shutter sound plays. Start the exe from any folder; it should play either way.
2. Start an area capture and press Esc to cancel it: the Windows default beep plays.
3. The unzipped artifact has no `Qt5Multimedia.dll`.
