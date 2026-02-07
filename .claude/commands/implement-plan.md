# Implement Plan

Execute the implementation plan step by step.

Plan file: $ARGUMENTS

If no file specified, look for the most recently modified `.md` file in `.claude/plans/`.

## Instructions

1. **Read the plan file** and parse all checklist items

2. **Identify current progress**:
   - `- [x]` = completed steps (skip these)
   - `- [ ]` = pending steps (execute these in order)

3. **Execute each pending step sequentially**:
   - Implement the changes described in the step
   - Verify the change works as expected
   - Mark the step complete by changing `- [ ]` to `- [x]`
   - Save the updated plan file after each step

4. **For verification steps** (linting, testing):
   - Run the specified commands
   - If checks fail: STOP and report the failure
   - Show the error output
   - Suggest fixes but wait for user confirmation before proceeding
   - Do not mark verification steps complete until they pass

5. **If a step fails or is unclear**:
   - STOP execution
   - Report what went wrong
   - Show relevant error messages or context
   - Ask for clarification before continuing
   - Do not skip steps or make assumptions

6. **Progress reporting**:
   - After each completed step, briefly report what was done
   - Show running progress: "Completed step X of Y"
   - At verification points, show full output

## Execution Rules

- **Never skip steps** - Execute in the exact order specified
- **Never assume** - If something is unclear, ask
- **Always verify** - Check that each change works before marking complete
- **Atomic updates** - Update the plan file after each step completion
- **Fail fast** - Stop immediately on errors, don't try to continue

## Handling Edge Cases

### If the plan file doesn't exist:
Report the error and suggest running `/create-plan` first.

### If all steps are already complete:
Report that the plan is fully implemented and summarize what was done.

### If a step references missing files:
Stop and ask if the file should be created or if the plan needs updating.

### If tests fail:
1. Show the test output
2. Identify which tests failed
3. Propose a fix
4. Wait for user approval before applying the fix
5. Re-run tests after fix
6. Only mark the test step complete when all tests pass

## Completion

When all steps are marked complete:
1. Provide a summary of everything implemented
2. List any files created or modified
3. Note any follow-up items or considerations
4. Remind user to commit their changes if appropriate
