# 11. Windows ships Store MSIX and a website EXE

Date: 2026-08-14

## Status

Accepted

## Context

[ADR 0010](0010-open-source-website-download.md) parked app stores for v1. Windows code signing for a site-hosted installer is a paid CA or Azure path; the Microsoft Store re-signs **MSIX** for free after certification. Many listeners also strip the Store out of Windows, so a Store-only Windows channel would strand them.

## Decision

Windows has **two** obtain paths:

- **Microsoft Store** — an **MSIX** package. Microsoft re-signs it. This is the clean install (no SmartScreen).
- **Official download** — an unsigned **EXE** installer on `https://tramp.music`, at the listener's acknowledged risk (SmartScreen click-through). This is the path when the Store is missing or unwanted.

tramp.music remains the product page and still hosts the EXE. Other stores stay out of v1 except Flathub ([ADR 0013](0013-linux-flathub-and-appimage.md)).

## Consequences

Two Windows artifacts and two update stories. The in-app prompt follows **install channel**: a Store install opens the Store product page; the website EXE opens tramp.music. The EXE is not Authenticode-signed; document SmartScreen. File associations must be declared in both the MSIX manifest and the EXE installer.
