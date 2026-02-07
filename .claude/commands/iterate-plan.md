# Iterate on Implementation Plan

Review and update the implementation plan based on user feedback.

Plan file: $ARGUMENTS

If no file specified, look for the most recently modified `.md` file in `.claude/plans/`.

## Instructions

1. **Read the plan file** specified above

2. **Find all feedback comments** marked with the pattern `[?? ... ??]`
   - These comments may contain:
     - Questions needing answers
     - Alternative approaches to consider
     - Requested fixes or modifications
     - Concerns about the approach
     - Requests for more detail

3. **Process each comment**:
   - For questions: Provide an answer and update the plan accordingly
   - For alternatives: Evaluate the suggestion, update if better or explain why not
   - For fixes: Apply the requested changes
   - For concerns: Address the concern, modify plan if needed
   - For detail requests: Expand the relevant section

4. **Remove the `[?? ... ??]` markers** after addressing each comment

5. **Preserve the checklist format** - Keep all checkbox syntax intact

6. **Add a "Changes Made" section** at the end of the plan documenting:
   - What feedback was received
   - How each item was addressed
   - Any follow-up decisions made

## Output Format

Update the plan file in place with all changes. The "Changes Made" section should look like:

```markdown
---

## Changes Made (Iteration [N])

| Feedback | Resolution |
|----------|------------|
| [Original comment] | [How it was addressed] |
| [Original comment] | [How it was addressed] |

**Summary**: [Brief description of major changes]
```

## After Updating

Inform the user:
- Summary of changes made
- Any new decision points that need their input
- They can add more `[?? comment ??]` markers and run `/iterate-plan` again
- When satisfied, run `/implement-plan` to execute
