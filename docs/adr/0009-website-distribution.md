# 9. Official download is the website; source stays private

Date: 2026-08-14

## Status

Superseded by [0010](0010-open-source-website-download.md) on the source posture. The website as **official download** and “no stores in v1” still hold.

## Context

v1 already said stores are not required. The remaining fork was how a listener actually gets Tramp: a public GitHub repo with Releases, or a public product on the web with the source kept private.

## Decision

Tramp is **Free Forever** (costs nothing to use) and **not open-source**. Listeners obtain binaries from the **official download** at `https://tramp.music`. The source repository is not a public distribution surface. App-store listings (Microsoft Store, Mac App Store, Flathub, Snap Store, etc.) stay out of v1.

How the binaries are signed, packaged, and updated, and how bundled libmpv is licensed relative to a closed-source Tramp, are follow-on decisions.

## Consequences

A website product has to survive Gatekeeper and SmartScreen; GitHub-sideload norms do not apply. File associations and installers become part of the download, not README recipes. A proprietary Tramp cannot ship a GPL libmpv build — that constraint is recorded when the libmpv pin is chosen, not here.
