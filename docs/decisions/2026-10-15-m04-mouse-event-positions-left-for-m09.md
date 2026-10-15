# 2026-10-15: M04, mouse event positions left for M09; QRegion::rects added

**Decision.** M04 does not touch `QMouseEvent::pos()`, `globalX()` and `globalY()` (`dialogs/areadialog.cpp`, `dialogs/previewdialog.cpp`, `tools/windowpicker.cpp`). They move to M09, next to the other calls Qt 5.15 cannot express in their Qt 6 form. M04 does replace `QRegion::rects()` in `AreaDialog::paintEvent`, which the plan's table missed, with `QVector<QRect>(mask.begin(), mask.end())`.

**Why.** The plan's replacement, `position()` / `globalPosition()`, exists only in Qt 6; M04 must build on Qt 5.15. The 5.15 alternatives (`localPos()`, `globalPos()`) are deprecated in Qt 6 too, so swapping to them would be churn that M09 redoes. The old calls still compile on Qt 6 (deprecated, not removed). `QRegion::rects()` is removed in Qt 6 and has a 5.15 form, so it belongs to M04.

**Also.** `Screenshot::unloadPixmap`'s temporary name used `qrand() * qrand() + time`; it now uses one `QRandomGenerator::global()->generate() + time` (a 32-bit random number already covers the range, and the old product overflowed `int`). `UGlobalHotkeys::unregisterAllHotkeys` iterates a copy of the registry, as `foreach` did, because `unregisterHotkey` removes from it.
