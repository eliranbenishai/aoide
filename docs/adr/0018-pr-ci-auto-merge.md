# Pull requests squash-merge only after Qt CI is green

Date: 2026-08-18

Pushing a feature branch opens a GitHub pull request. Ubuntu and Windows Qt jobs are the review gate; a PR comment records the result. The PR squash-merges only when that gate is green — not when CI is red, draft, from a fork, or labeled `do-not-merge`.

## Status

Accepted

## Decision

GitHub Actions owns the loop: **Open PR** on push (not `main`, `research/*`, `spike/*`, `wip/*`), **CI** on the pull request (review comment plus squash-merge when green), **Merge if green** from the default-branch `workflow_run` as a second gate so a later PR cannot delete the merger by editing only `ci.yml`. No human click to merge a green same-repo PR. Human review stays available; it is not required for merge.

## Considered options

- Require a human approving review — rejected for a solo maintainer (you cannot approve your own PR, so the loop would stall).
- Merge from the PR’s own `ci.yml` — rejected; a PR could rewrite the gate. `workflow_run` always uses the workflow on `main`.
