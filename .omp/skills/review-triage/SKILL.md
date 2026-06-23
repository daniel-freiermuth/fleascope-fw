---
name: review-triage
description: Evaluate a detail.dev code review comment against the actual codebase. Determines whether a finding is valid, outdated, a false positive, misleading, or needs action.
---

# Review Triage

You are triaging a code review comment from detail.dev. Your job is to give an honest, grounded verdict — not to blindly accept or reject it.

## Input

The user will paste one review finding. It will typically include:
- A file path and line number (or range)
- A severity / category label
- A description of the alleged problem
- Sometimes a suggested fix

## Process

Work through these checks in order. Stop at the first one that fully explains the finding.

### 1. Read the actual code

Read the file at the referenced location. If no line is given, search for the symbol or pattern mentioned.

**Check: does the finding still describe what the code actually does?**
- If the code was already fixed or refactored → **OUTDATED**
- If the line numbers are off and the real code is different → **OUTDATED**

### 2. Understand the intent

Look at surrounding context: adjacent lines, the function/struct it lives in, any relevant comments. Check git log on that file for recent changes (`git log -10 --oneline -- <file>`).

**Check: does the reviewer correctly understand what the code is trying to do?**
- If the reviewer misread the logic, confused a variable name, or missed a guard clause above → **MISLEADING**
- If the "bug" is actually the intended behavior for a documented reason → **MISLEADING**

### 3. Assess actual risk

Evaluate whether the described problem can actually cause harm in this codebase:
- Is the code path reachable?
- Is the input untrusted or always controlled?
- Is there a higher-level guard that makes this moot?
- Is this embedded / bare-metal firmware where different rules apply (e.g. no OS, fixed inputs, no heap)?

**Check: can the described scenario actually occur?**
- If the risk requires conditions that cannot arise → **NOT A PROBLEM**
- If this is a theoretical issue with negligible real-world impact → **LOW PRIORITY**

### 4. Check for duplicates or coverage elsewhere

Search the codebase for existing handling of the same issue. Check if there's a related comment, assert, or test that already addresses it.

**Check: is this already handled?**
- If yes → **NOT A PROBLEM** (already mitigated)

### 5. Evaluate the fix

If a fix is suggested, assess whether it's correct, idiomatic, and safe in this context. A bad suggested fix is a strong signal the reviewer misunderstood.

---

## Verdict labels

| Verdict | Meaning |
|---|---|
| **VALID** | Real problem, code should be changed |
| **VALID — LOW PRIORITY** | Real but cosmetic / minor; fix opportunistically |
| **OUTDATED** | Finding no longer matches the current code |
| **NOT A PROBLEM** | Technically correct but no actual risk in this context |
| **MISLEADING** | Reviewer misunderstood the code or its intent |
| **NEEDS CONTEXT** | Cannot determine without more information (state what's missing) |

---

## Output format

```
**Verdict:** <label>

**Evidence:** <1–3 sentences citing exactly what you read in the code>

**Reasoning:** <why the verdict follows from the evidence>

**Action:** <what to do — fix it / close it / add a comment explaining intent / ask reviewer for clarification>
```

Keep it short. Evidence must be grounded in what you actually read, not what you assumed.
