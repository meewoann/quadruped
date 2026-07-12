---
description: Summarize commits on the current branch (vs origin/main) into a Changelog-style bullet list ready to paste into Changelog.md.
argument-hint: "[base-ref, default origin/main]"
allowed-tools: ["Bash"]
---

Produce a Changelog-friendly summary of the current branch.

1. Determine the base ref: `$ARGUMENTS` if provided, else `origin/main`.
2. Show what's about to be summarised:
   `git log --oneline <base>..HEAD`
3. For each commit, classify it as one of:
   * **Features** (new public API or system),
   * **Fixes** (bug fix referenced by issue/PR),
   * **Documentation**,
   * **Infrastructure / CI**,
   * **Refactor / Cleanup**.
4. Group commits under those headings and produce bullets in the same
   style as the existing `Changelog.md`
   (`* Short imperative summary  \n    * [Pull request #NNNN](...)`).
5. Print the markdown block — do **not** edit `Changelog.md` automatically.
   Ask whether to insert it under the unreleased / upcoming section.
6. Flag any commits that look like they need a `Migration.md` note
   (API removals, default changes, plugin renames).
