## Agent skills

### Auto-apply

Every session: match the task to installed skills and apply them (including user-invoked Matt skills like `to-spec` / `implement` even when they disable model auto-invocation). Prefer the Matt Pocock engineering flow over Superpowers when both cover the same step. If the match is unclear, use `ask-matt`; if that finds nothing appropriate, proceed without a saved skill. See `.cursor/rules/apply-skills.mdc`.

### Issue tracker

Issues live as local, gitignored markdown under `.scratch/<feature>/` — do not commit them. Lasting decisions go in `CONTEXT.md`, `docs/adr/`, and `docs/architecture.md`. See `docs/agents/issue-tracker.md`.

### Triage labels

Default vocabulary: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context — root `CONTEXT.md` + `docs/adr/`. See `docs/agents/domain.md`.

### ADR contradictions

Cardinal: if a request would violate an accepted ADR, stop, cite the clash, and wait for instructions. Do not implement around it or quietly rewrite the ADR. See `.cursor/rules/adr-contradictions.mdc`.

### Compositor geometry

Offscreen tests are not proof the host window moved. The host is the virtual desktop; title-bar drags are app-owned in host-local space. See `.cursor/rules/compositor-geometry.mdc`.

### Architecture

Living design map: `docs/architecture.md`. Update it in the same change whenever the app’s structure, boundaries, stack, or major features change. See `.cursor/rules/architecture-doc.mdc`.

### Subagent delegation

Cardinal default: dispatch independent work to subagents in parallel; the parent coordinates and integrates. See `.cursor/rules/delegate-subagents.mdc`.

### Subagent models

Do not use Claude or ChatGPT/GPT models for subagents unless the user explicitly requests them, or a task truly requires them and the user approves. See `.cursor/rules/subagent-models.mdc`.

### Commits

When a discrete task is finished, **commit it** (do not wait for a separate “please commit”). See `.cursor/rules/commit-completed-tasks.mdc`.
