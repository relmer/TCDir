# TCDir → RCDir Feature Sync Status

RCDir is a [Rust port of TCDir](https://github.com/relmer/RCDir).
New features land in TCDir first, then get ported to RCDir.

This document tracks which features have been ported and which are pending.

## Sync Table

| Spec #   | Feature                | TCDir Status       | TCDir Version | RCDir Status       | RCDir Version | Notes                                                  |
| --- | ---------------------- | ------------------ | ------------- | ------------------ | ------------- | ------------------------------------------------------ |
| 001 | Cloud File Status      | Shipped            | v4.2          | Shipped            | v5.0          | Folded into RCDir's 002 port spec (no separate 001)    |
| 002 | Full Application Spec  | Shipped            | v4.2          | Shipped            | v5.0          | Complete port — 100% output-parity target               |
| 003 | Nerd Font File Icons   | Shipped            | v5.0.1038     | Shipped            | v5.0          | Ported from TCDir's 9-phase implementation              |
| 004 | Tree View Display Mode | Shipped            | v5.1.1106     | Shipped            | v5.1.1132     | 64 tasks, 314 tests — output-parity verified            |
| Fix | Pure Mask Dir Fix      | Shipped            | v5.1.1149     | Shipped            | v5.1.TBD      | `tcdir x64` now lists dir contents like `tcdir .\x64`  |

## Workflow

1. New feature is specified and implemented in TCDir
2. Copy the spec directory to `RCDir/specs/` once TCDir implementation ships
3. Run `/speckit.plan` in RCDir to generate a Rust-specific plan
4. Implement and validate with output-parity tests
5. Update this table when the RCDir port ships

## Current Versions

| Project | Latest Release | Date       |
| ------- | -------------- | ---------- |
| TCDir   | v5.1.1149      | 2026-03-05 |
| RCDir   | v5.1.1132      | 2026-02-28 |
