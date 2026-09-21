# Aoide agent instructions

These repository-wide instructions are the canonical migration of `.cursor/rules/`
and the workflow preferences in `.cursor/permissions.json`. Cursor rule files
point here; maintain the rules in this file. Explicit user instructions override
these project defaults. Follow the active environment's higher-priority tool,
sandbox and approval policies.

## Apply saved skills

<!-- Migrated from apply-skills.mdc. -->

At the start of each turn, and when intent becomes clear, match the task to
installed skills and use a clear match without waiting for an explicit invocation.
Read the skill, briefly announce its use, then follow it. Apply process skills
before implementation skills and choose one workflow per step.

Look in this order:

1. The session's available-skills catalog.
2. User skills at `~/.agents/skills/*/SKILL.md` (also exposed through
   `~/.cursor/skills/` in Cursor).
3. Available application/plugin skills, including `~/.cursor/skills-cursor/`
   when working in Cursor.
4. Project guidance in `AGENTS.md` and `docs/agents/`, when present.

Prefer the Matt Pocock engineering flow on overlap:

| Task | Preferred skill | Avoid stacking with |
| --- | --- | --- |
| Sharpen an idea or design | `grill-me` → `grilling` | `brainstorming`, `grill-with-docs` |
| Turn discussion into a tracker spec | `to-spec` | A new `docs/superpowers/specs/` design by default |
| Break a spec or plan into tickets | `to-tickets` | `writing-plans` for new work |
| Implement a spec or tickets | `implement`, with `tdd` at appropriate seams | `executing-plans`, `subagent-driven-development`, unless the user selects an existing Superpowers plan |
| Diagnose a hard bug or unexpected failure | `diagnosing-bugs` | `systematic-debugging` |
| Review against standards and spec | `code-review` | `requesting-code-review`, `receiving-code-review` |
| Author skills or agent instructions | `writing-for-agents` | `writing-skills` |
| Work on domain language | `domain-modeling` | Using it as a side effect of grilling |
| Navigate a large multi-session effort or uncertain codebase | `wayfinder` | An invented replacement workflow |

The main flow is `grill-me` → `to-spec` → `to-tickets` → `implement` →
`code-review`; use the steps relevant to the task. Superpowers is appropriate
where Matt has no counterpart, such as `using-git-worktrees`,
`finishing-a-development-branch`, `verification-before-completion`, and
`dispatching-parallel-agents`.

If the match is unclear, read `~/.agents/skills/ask-matt/SKILL.md` and use it as
the router, substituting `grill-me` wherever it recommends `grill-with-docs`.
If no installed skill fits, proceed with normal judgment. Reserve `find-skills`
for requests to discover or install skills, not routing among installed skills.
An explicit request to use or skip a skill wins. A skill's
`disable-model-invocation: true` flag does not cancel this project's instruction
to use a matching installed skill, subject to the host's invocation rules.

Keep existing `docs/superpowers/` material valid as historical/design context.
For new tracker work, consult `docs/agents/issue-tracker.md` when available;
tracker state belongs in local, gitignored `.scratch/`. That guidance directory
may be absent in a checkout: report missing required guidance instead of
pretending to have read it.

## Sharpen ideas through an interview

<!-- Migrated from grill-me-only.mdc. -->

Use `grill-me` for sharpening a plan, design or idea: it invokes the `grilling`
interview. If the host lacks a Skill tool, read and follow the installed
`grilling` skill directly. This rule also applies to auto-matching and router
recommendations.

Grilling alone produces an interview, without a paper trail. Record lasting
decisions in `CONTEXT.md` or `docs/architecture.md` only when the user asks to
write them down; do not create ADRs or update the glossary as a grilling side
effect. An explicit `grill-with-docs` invocation or “write this down” overrides
this default for that turn. Implemented architecture changes still require the
documentation update below.

## Keep architecture documentation current

<!-- Migrated from architecture-doc.mdc. -->

Update `docs/architecture.md` in the same change when adding, removing or
renaming a major module, package or boundary; changing interactions between
layers; introducing or replacing an architecture-shaping dependency; making or revising
a structural stack/platform decision; or adding components or data flow.
Tiny local changes such as typos, styling or comments do not require an update.

Keep it a map of structure and responsibilities, using concise sections and
Mermaid diagrams where useful. Inspect the implementation when uncertain.
If the document is missing, recreate it from the current code before finishing.
Use `CONTEXT.md` for domain vocabulary.

## Respect compositor geometry

<!-- Migrated from compositor-geometry.mdc; updated with user approval to ADR 0001. -->

Before changing host windows, panel movement, docking or geometry, read
`docs/adr/0001-platform-window-presentation.md` and the Host section of
`docs/architecture.md`.

Cocoa, Windows and X11 use panel-sized native windows. Wayland uses a bounded,
opaque container with embedded panels; the compositor owns whole-window
movement. Keep embedded-panel movement and docking in container coordinates,
and native layouts in screen coordinates. Use actual mapped geometry and
`mapToGlobal()` when translating coordinates. Avoid repeated `setGeometry()`
calls after mapping when the desired rectangle has not changed.

The retired virtual-desktop-sized transparent host must not be reintroduced.
App-owned embedded-panel drags move the panel widgets; whole-host movement may
use the compositor's system-move operation, as the current architecture requires.

Offscreen QtTest honors requested window positions that Wayland/KWin may ignore.
A passing offscreen `setGeometry()` assertion is not proof the OS host moved.
Check panel/sibling geometry independently of arbitrary top-level positioning,
and verify compositor-dependent behavior on the relevant desktop before claiming
it works there.

## Delegate independent work

<!-- Migrated from delegate-subagents.mdc. -->

Delegate independent work to subagents when the environment supports them.
Dispatch in parallel when two or more tasks have separate files and no sequential
dependencies. Give each agent a self-contained scope, constraints, required
files/docs and expected report. Prefer exploration or implementation agents for
codebase reads, diagnosis slices and isolated edits. Use the installed
`dispatching-parallel-agents` skill for prompt structure when available.

Keep product-language decisions, design confirmation, conflicting edits and
final integration with the parent. Follow the model policy below. If delegation
is unavailable, continue locally rather than blocking the task.

## Select subagent models deliberately

<!-- Migrated from subagent-models.mdc. -->

Prefer inheriting the session's default model without a model override, or an
available non-Claude/non-GPT option such as Composer or Grok. Do not explicitly
select a Claude or ChatGPT/GPT-family model unless the user names or approves
that model for the run. If a capability gap requires such an explicit selection,
explain why in one or two sentences and obtain approval before launching it.
Inheriting the default is the original rule's permitted default-model path.

## Write listener-facing release notes

<!-- Migrated from release-notes.mdc. -->

Before writing version notes, read **Release notes** in `docs/distribution.md`;
release 1.2 is the reference. List capabilities a listener can notice, with a
concise explanation after a dash only where needed. Keep engineering changes
and standing product facts such as download channels or notarization out of
per-version notes.

Write notes once, in that version's `<release>` description in
`packaging/linux/com.proximamagnifica.aoide.metainfo.xml`, as a `<ul>` of `<li>`
features. `tool/release-notes.sh` derives the GitHub body; do not maintain a
separate release body.

## Commit completed tasks

<!-- Migrated from commit-completed-tasks.mdc. -->

Create a Git commit when a discrete task is finished, without waiting for a
separate commit request. Completion means the agreed implementation or document
delivery is written, relevant verification has run, and required architecture
or spec updates are included. Prefer one commit per task.

Stage only that task's changes. Exclude unrelated work, scratch fidelity dumps
and secrets. Follow Git safety conventions: no force pushes or configuration
changes; use a heredoc or message file for multiline commit messages, and verify
`git status` afterward. Push only when the user asks.

An explicit instruction to leave work uncommitted wins. If completion itself
is ambiguous, ask once and commit after the user confirms it is done.

## Tool approval preferences

<!-- Migrated from permissions.json; guidance, not executable sandbox configuration. -->

Workspace-local shell commands for building, testing, Git and other project work
may proceed within the active sandbox. The original Cursor preferences require
approval for MCP calls, web search/fetch, network fetches and remote HTTP
requests. Carry that approval preference into this environment where supported;
existing user authorization in the session remains valid. If approval is needed,
identify this section as its source and explain the action before asking.

These written preferences do not configure or bypass the host's actual tool
permissions. Keep application-specific permission settings in that application;
do not rewrite global approval or sandbox settings as a side effect of a task.
When configuring Cursor, prefer its Auto-review run mode; this preference does
not select or change another application's approval mode.
