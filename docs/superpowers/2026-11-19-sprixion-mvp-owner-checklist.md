# Sprixion MVP — owner checklist and build decisions

Agents could not see the GUI or run Unity/Windows. Everything below needs a human. Grouped by plan task.

## Task 9
- [ ] Draw with every tool (pencil, eraser, fill, eyedropper, select, line, rect, ellipse), including mirror H/V and pixel-perfect; undo and redo each one
- [ ] Zoom with the wheel (zooms around the cursor) and pan (middle-drag, Space+drag)
- [ ] Paste a 64×64 PNG from the system clipboard into a 32×32 sprite: it is clipped, no crash
- [ ] Save, reopen and compare
- [ ] Open a renamed `.txt` as `.sprx`: an error dialog appears and the previous document stays open
- [ ] Export a sheet and inspect the JSON (`meta.image` = the PNG's file name, frames/durations present)


## Task 10
- Hide a layer via its checkbox; canvas updates.
- Drag the opacity slider; exactly one undo entry; undo restores the pre-drag opacity.
- Import the DB32 .gpl from Lospec; swatches fill; left/right click set primary/secondary.
- Type a hex color (#RRGGBB and #RRGGBBAA); draw with it; invalid text reverts.
- Deleting the only layer does nothing.
- Add layer (above current), move up/down, rename by double-click, undo each, including undo of a delete/add with a selected layer.
- X swaps colors; clicking an overlapping swatch picks which is edited; eyedropper updates the picker.
- File > New / Open: all panels rebind.


## Task 11
- [ ] 8-frame walk with tag `walk` (frames 1–8) plays at the per-frame durations (set some to different ms via header double-click; verify tooltip shows them).
- [ ] "Tag Only" loops just the tag range; with Loop off it stops on the tag's last frame.
- [ ] Onion skin (F3 or toolbar) shows faded neighbors; Before/After spin boxes change how many.
- [ ] Deleting frames inside a tag shrinks it (and deleting its only frame removes it).
- [ ] 256×256, 10 layers, 100 frames plays smoothly.
- [ ] Enter toggles play/pause when canvas or timeline has focus; Alt+, / Alt+. step; Enter in the hex field / layer rename still commits.
- [ ] Drawing on the canvas pauses playback.
- [ ] Clicking a timeline cell selects the same layer in the Layers panel and vice versa; undo of "Add Frame" while on the new frame leaves a valid current frame.
- [ ] Multi-frame duration edit (shift-click headers, double-click one, or right-click → Frame Duration…) is one undo step.
- Known behavior: Enter plays only when the canvas or timeline has focus.


## Task 12 (autosave)
- [ ] Draw, wait for an autosave (3 min), kill -9 the app, relaunch: the recovery dialog restores the drawing
- [ ] A normal close leaves the recovery folder empty
- [ ] Closing / New / Open with unsaved changes asks Save / Don't Save / Cancel

## Task 14 (packaging, CI)
1. Approve and create the GitHub repo, add remote, push feat/mvp.
2. Check both CI jobs green; if linux job's AppImage fails to start headless, see concern above.
3. Download artifacts; run the AppImage on Linux (real display) and check icons render (SVG plugin).
4. On Windows: unzip and run sprixion.exe, run the setup.exe, check Start-menu shortcut, icons, uninstall.
5. Visually check packaging/linux/sprixion.png.


## Task 15 (Unity importer)
- [ ] M0: The package compiles with no errors in both Unity versions. This was never compiled here.
- [ ] M1: Save `walk.sprx` (8 frames, tag `walk` 0–7, 100 ms each, frame 3 = 200 ms) into `Assets/`. Expect a texture with 8 crisp, unfiltered sprites and a `walk` clip of 0.9 s. Put the first sprite in the scene, add an Animator with the clip, press Play: it animates.
- [ ] M2: Save `tiles.sprx` (64×64, slice grid 16×16, 3 empty cells). Expect 13 sprites and no clips.
- [ ] M3: Delete 4 frames and the tag in Sprixion, then re-save over `walk.sprx`. Expect 4 sprites, no `walk` clip, no stale sub-assets, no console errors.
- [ ] M4: Rename a `.txt` to `bad.sprx` in `Assets/`. Expect one import error in the console and no exception spam.


## Final-review fixes
Item 1:
1. Draw something, select an area elsewhere, Ctrl+C, then Ctrl+V. Ctrl+Z should remove only the pasted pixels; the earlier stroke must remain. A second Ctrl+Z undoes the stroke.
2. With the Select tool, drag the inside of a selection to lift and move it, toggle another layer's visibility, then press Ctrl+Z. The toggle should be undone and the moved pixels should stay where they were dropped; nothing should vanish. Repeat with a layer rename, an opacity change, palette add/remove, a tag or duration edit, and the slice grid.
3. Lift and move a selection, then press Ctrl+Z without other edits. The pixels should return to their original place (the lift is undone).
4. Select an area, switch to Pencil, press Flip Horizontal. The tool should switch to Select. Draw nothing, press Ctrl+Z: the flip and the lift revert.
5. Cut (Ctrl+X): the area clears, no marching ants remain, and Ctrl+Z restores it.
Item 2:
1. Edit a sprite, wait more than 3 minutes for an autosave, then kill the app (`kill -9`). Relaunch and choose Restore. Without waiting, kill it again. Relaunch: the recovery prompt must appear again with the same work.
2. Choose Discard instead: the prompt must not reappear at the next launch.
Item 4: in a new untitled sprite, choose Export Frame. The suggested path should be ~/untitled.png.

## Decisions made during the build (rulings)
- ## Rulings
- - Ruling: all core types incl. Document live in namespace `spx`; tests may `using namespace spx;` — plan mixes qualified/unqualified — cost if wrong: trivial rename.
- - Ruling: T1 skips steps 1-2 (done); T1 adds `.superpowers/` to .gitignore — workspace must stay untracked — cost: none.
- - Ruling: T6 POSIX-permission test QSKIPs on Windows and when geteuid()==0 — plan only mentioned Windows — cost: one skipped test.
- - Ruling: GUI manual checks (T9–T12) → implementer verifies build + offscreen launch (`QT_QPA_PLATFORM=offscreen`, exits clean via a `--smoke` hidden flag? NO — no extra flags; use `timeout 3` and check no crash output) and writes the manual checklist into its report; checklists are handed to the owner at the end — spec can't be verified by agents visually — cost: GUI bugs found later by owner.
- - Ruling: T14 does not push or create a GitHub repo; CI workflow written but unverified; owner pushes later — publishing needs owner consent — cost: CI bugs surface on first push.
- - Ruling: T15 manual Unity checks M1–M4 deferred to owner — no Unity here — cost: importer bugs found later.
- - Ruling: miniz fetched from its official GitHub release (MIT) by T1 implementer — vendoring decided in spec — cost: none.
- Task 2: implemented 35c46ac (DONE_WITH_CONCERNS: no RED evidence; mirrored() deprecation note; trailer says Sonnet). Ruling: subagent co-author trailer naming its own model is accurate — accept — cost: none.
- Task 2: review Approved (no Crit/Imp). Ruling: resumed implementer for minor hardening (rotation direction test, ARGB32 format invariant, deprecation noise, move-op tests) — later tasks rely on format/direction — cost: one small round.
- Task 3: implemented 339844d, review Approved. Ruling: resumed implementer for exact partial-alpha restore test (+ fix if drift) and multi-stroke/snapshot tests — undo must be lossless — cost: small round.
- Task 5: implemented a13c282; review dispatched. Ruling: commit trailer names the committing agent's own model (global-constraints updated) — accurate attribution — cost: none.
- Ruling (owner request 2026-11-19): ponytail skill mandatory for all implementers/reviewers from Task 5 review onward; Tasks 1-5 code gets a ponytail-review pass during final review.
- Ruling: load caps — layers ≤256, frames ≤4096, manifest ≤16MB, PNG entry ≤2048*2048*4+1MB, decoded cel total ≤2GiB, duplicate cels rejected — spec silent; generous vs perf target (10 layers/100 frames) — cost if wrong: a huge legit file refuses to open; caps are constants, easy to raise.
- Task 7: fix round 1/5 (2 addressed, 0 open; commits de05a42..fd63442). Ruling: 10-line fix diff re-reviewed by controller directly instead of a subagent — trivially verifiable — cost: low.
- Ruling (carried to Task 9): floating-selection contract — before undo/redo: discard floating (`sel.floating = {}`), then undo (reverts the lift → original pixels); before layer/frame switch, save, export, tool switch: dropSelection — floating isn't bound to layer/frame and undo doesn't know about it — cost if wrong: duplicated/lost pixels.
- Task 9: implemented aa7f9d2. Ruling: Edit Flip/Rotate with an active (non-floating) selection lifts and transforms only the selected pixels — matches user expectation in pixel editors; brief's "current cel" applies when no selection — cost: none.
- Task 12: implementer agent a11bedd9bf9c31e1d (sonnet, ponytail). Rulings: autosave never settles floating selection (shortcut comment); untitled autosaves w/ originalPath ""; recovery dialog newest-first, first Restore stops asking, rest stay; no instance lock (shortcut comment) — cost if wrong: 2nd instance may offer a live session's file; prompt guards close/New/Open/import.
- Task 13: dispatched (base 2b94af2, sonnet, ponytail, agent a1bbf035a34b16d57). Ruling: fake locale uses tr_TR instead of xx (xx resolves to C); main.cpp may switch to QTranslator::load(QLocale(),"sprixion","_",":/i18n") for language fallback — cost: none.
- Ruling: Task 13 task review done by controller instead of a subagent — non-generated diff is 1 line in main.cpp; controller spot-grepped src/app for untranslated literals (none) and verified no throwaway files remain — cost if wrong: a missed string, caught in final review.
- Task 14: dispatched (base bedcad5, sonnet, ponytail, agent a040b2fa047799220). Rulings: no push/remote (owner step); linuxdeploy downloaded to git-ignored cache; local AppImage only runs on new glibc — CI ubuntu-22.04 builds the release one; Windows scripts unverified; svg plugins must ship.
- Task 15: dispatched (base af7194a, opus — no C# compiler/Unity here, correctness by reading; agent ac91ba476656700a7). Rulings: package LICENSE GPLv3 (editor-only asmdef, nothing ships in game builds); tag direction forward-only (shortcut); atlas >16384 → import error; FullRect sprites; manifest validated as trust boundary; JsonUtility DTOs, no deps.
- Ruling: final fix wave = 4 Important + ponytail minors 5-9 (shortcut comment, smoke test, flip #if, QSet dedup, redundant check/includes); left: empty cels from no-op tools, PNG re-deflate in autosave, duration bound mismatch, setLayerProps signature, m_dragging dup — all cosmetic/perf-unmeasured — cost if wrong: small polish later.

## Known limits left as-is
- Task 6: minor (deferred): colorFromStr lenient on "0x"/whitespace; readEntry 2x peak copy.
- Task 6: minor (deferred): loadSprx readAll() on whole archive with no file-size cap; no tests for PNG entry cap / 2GiB total.
- Task 7: minor (deferred): Fill/Shape double copy; mirror size source differs (sprite.size vs cel.size).
- Task 8: minor (deferred): SheetExport drawImage SourceOver vs Source precision; PaletteIO size cap bypass for FIFOs; weak e.contains("1") assertion.
- Task 9: minor (deferred): undo after paste also undoes previous command (contract consequence); selection rect lingers after discard; full recomposite incl. onion on every changed; wheel zoom doubling jumps on touchpads; ants timer always on; export path default in cwd.
- Task 10: minor (deferred): rowChanged double rebuild; m_dragging dup of isSliderDown; SV square per-pixel repaint on every contextChanged; no parseHex test (no app test target); mid-drag doc replace edge.
- Task 11: minor (deferred): MainWindow assembles timeline toolbar (createMenus depends on createDocks order); context-menu frame target during playback; timeline doesn't autoscroll to playing frame.
- Task 12: minor (deferred): no unit test for Autosave statics (pendingRecoveries/info/discard testable with QTemporaryDir); .ts not regenerated (Task 13).
- Task 13: minor (deferred): .ts carries line numbers → churn on every lupdate (could use -locations none).
- Task 14: minor (deferred): double build in CI (build/ + build-release/); linuxdeploy downloads unchecksummed (accepted by ruling); Inno Setup version on runner (x64compatible needs ≥6.3) unverified.
- Task 15: minor (deferred): same-name tags reorder swaps clip ids; tiny slice grid could create huge sprite counts; M4 error text raw.
- Final fix wave re-review: all 9 addressed, no new breakage (commits e8148be..e6bd1aa). Out-of-scope: no vertical-flip pixel test; paste into fresh doc can't be undone by Ctrl+Z (Esc works) — pre-existing.
