## Agent skills

### Auto-apply

Every session: match the task to installed skills and apply them (including user skills like `to-spec` / `implement` even when they disable model auto-invocation). If the match is unclear, use `ask-matt`; if that finds nothing appropriate, proceed without a saved skill. See `.cursor/rules/apply-skills.mdc`.

### Issue tracker

Issues live as local markdown under `.scratch/<feature>/`. See `docs/agents/issue-tracker.md`.

### Triage labels

Default vocabulary: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context — root `CONTEXT.md` + `docs/adr/`. See `docs/agents/domain.md`.

### Architecture

Living design map: `docs/architecture.md`. Update it in the same change whenever the app’s structure, boundaries, stack, or major features change. See `.cursor/rules/architecture-doc.mdc`.

### Subagent models

Do not use Claude or ChatGPT/GPT models for subagents unless the user explicitly requests them, or a task truly requires them and the user approves. See `.cursor/rules/subagent-models.mdc`.
