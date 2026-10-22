# PNG optimizers after S05 (S05, bug 10)

Use the `lightscreen-win64` artifact of the latest `master` CI run (it contains `oxipng.exe`, `optipng.exe` and a `licenses` folder), with Options tab > Screenshots > "Optimize PNG screenshots." on and format PNG (General tab).

1. **oxipng.** Choose "oxipng" in the box next to the checkbox, save, take a full-screen screenshot: the file ends up smaller than the same screenshot with optimization off, and opens fine.
2. **OptiPNG.** The same with "OptiPNG".
3. **Fallback.** Close Lightscreen, rename `oxipng.exe` to `oxipng.exe.off`, start it again: the box shows "oxipng (not found)"; with oxipng chosen a screenshot is still optimized (by OptiPNG). Rename it back.
4. **Burst.** Take five screenshots quickly: the tray tooltip shows "N optimizing" counting down, Task Manager shows at most one oxipng or optipng process at a time, and every file ends up optimized.
5. **Restart.** Take a burst of screenshots and quit Lightscreen at once from the tray menu: the files are intact (they open). Start Lightscreen again: the remaining files get optimized ("N optimizing").
