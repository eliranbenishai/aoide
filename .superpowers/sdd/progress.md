# Tramp v1 SDD progress ledger

Plan: docs/superpowers/plans/2026-08-01-tramp-v1.md
Branch: feat/tramp-v1
Worktree: .worktrees/tramp-v1
Started: 2026-08-01

Task 1: complete (commits d2ac76f..9d814f0, review had smoke-run gap; closed after VS repair + rustup; windows debug build + 5s smoke OK 2026-08-01)
Minor carry-forward: README still stock Flutter boilerplate

Task 2: complete (commits 9d814f0..69d41ed, review clean)
Task 3: complete (commits 69d41ed..c197b72, review clean)
Task 4: complete (commits c197b72..bac7b1e, review clean)
Task 5: complete (commits bac7b1e..ece59e8, review clean after persistence tests)
Task 6: complete (commits ece59e8..a260bfe, review clean)
Task 7: complete (commits a260bfe..6cb9d9e, review approved with manual-verify caveat; smoke launch OK)
Task 8: complete (commits 6cb9d9e..5827504, review clean)
Task 9: complete (commits 5827504..e27d306, review clean after metadata path fix)
Task 10: complete (commits e27d306..4aba3a3, review clean)
Task 11: complete (already satisfied by prior restore wiring + Task 5 tests; no new commit)
Task 12: complete (commits 4aba3a3..6bb0f2d, shortcuts+semantics)
Task 13: complete with concerns (7754f1d; Windows SMTC + macOS channel; Linux MPRIS stub TODO)
Task 14: complete (f444000; second-instance redirect deferred)
Task 15: complete (a6b9c88; windows release build + docs)

## Whole-branch review fixes (2026-08-01)

- Playback desync: `playingIndex` vs `selectedIndex`, playPause opens selected, remove-while-playing advance/stop
- Docs: README + architecture known v1 limits (Linux MPRIS, second-instance, macOS/Linux smoke)
- Tests: 32 pass; `flutter build windows --debug` OK
- Report: `.superpowers/sdd/final-fix-report.md`
