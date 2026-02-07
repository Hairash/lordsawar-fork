# Create Implementation Plan

Create a detailed implementation plan for the following task:

$ARGUMENTS

## Instructions

Generate a comprehensive implementation plan as a Markdown checklist. The plan must include:

### Structure Requirements

1. **Summary** - Brief description of what will be implemented
2. **Files to Modify/Create** - List all files that will be touched
3. **Implementation Steps** - Ordered checklist with:
   - All implementation steps in logical order
   - Specific code changes needed (files, functions, models)
   - Each step should be atomic and verifiable
4. **Decision Points** - For any non-obvious choices:
   - Proposed solution
   - Trade-offs considered
   - Rationale for the choice
5. **Testing Steps** - Include:
   - Unit tests to write/update
   - Integration tests as appropriate
   - Manual verification steps if needed
6. **Verification Steps** - Must include:
   - [ ] Run linter and fix any issues
   - [ ] Run test suite and verify all tests pass
7. **Documentation Updates** - Final steps:
   - [ ] Update CLAUDE.md if new patterns or conventions introduced
   - [ ] Update relevant skills/commands if workflow changed
   - [ ] Add inline code comments where logic is non-obvious

### Output Format

Save the plan to `.claude/plans/<descriptive-name>.md` (create the `.claude/plans/` directory if it doesn't exist). Use a descriptive filename based on the task, e.g., `add-multiplayer-chat.md`, `fix-memory-leak.md`.

Use this format:

```markdown
# Implementation Plan: [Brief Title]

## Summary
[2-3 sentence description]

## Files to Modify
- `path/to/file1.cpp` - [brief description of changes]
- `path/to/file2.h` - [brief description of changes]

## Implementation Steps

### Phase 1: [Phase Name]
- [ ] Step 1 description
  - Details about what to change
  - Code snippet if helpful
- [ ] Step 2 description

### Phase 2: [Phase Name]
- [ ] Step 3 description

## Decision Points

### Decision 1: [Brief description]
- **Proposed**: [chosen approach]
- **Alternative**: [other option considered]
- **Trade-offs**: [why this choice was made]

## Testing

- [ ] Write unit test for [specific functionality]
- [ ] Update integration test for [specific scenario]
- [ ] Manual test: [description]

## Verification

- [ ] Run linter: `make lint` or equivalent
- [ ] Run tests: `make check` or equivalent
- [ ] Verify build succeeds

## Documentation

- [ ] Update CLAUDE.md if needed
- [ ] Update command/skill files if workflow changed
- [ ] Add code comments where needed
```

After creating the plan, inform the user that they can:
- Review and add comments using `[?? comment ??]` syntax
- Run `/iterate-plan` to process their feedback
- Run `/implement-plan` to execute the plan
