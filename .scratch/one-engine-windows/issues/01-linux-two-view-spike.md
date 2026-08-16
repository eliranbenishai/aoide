# 01 — Linux two-view drag spike

Status: resolved
Type: prototype

## Question

With Impeller off, is title-bar drag still buttery when a **second Flutter view** lives on the **same engine**, sitting still (not following)?

## Run

`tool/run_windowing_spike.sh`

Two frameless windows. Drag MAIN's gray bar. OTHER should not move.

## Answer

Not slow (unlike five engines). Movement was **choppy**, not cursor-lag. That chop is Flutter `RegularWindow` rebuilding on every GTK configure during native drag — not extra-view cost. Solo-main on the same machine was buttery with OS drag and no configure rebuild.

Proceed: one engine, several views; do **not** use `RegularWindow`’s listen-on-move path in the product host.
