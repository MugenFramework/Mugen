# Contributing to Mugen

All contributions are welcome. Please follow these guidelines.

## Submitting Changes

Create a new branch and open a pull request against `main`:

```bash
git switch -c my-feature-branch
# make changes
git push --set-upstream origin my-feature-branch
```

Then open a pull request on GitHub.

## Reporting Bugs

Use the GitHub issue templates:

- **Demon bug** - issues with the Demon agent
- **Teamserver/Client bug** - issues with the teamserver or client UI
- **Documentation bug** - errors in the docs

## Feature Suggestions

Open a Feature Suggestion issue. Include a clear description and any relevant references or code samples.

## Code Style

- Teamserver (Go): follow standard Go conventions
- Client (C++/Qt): follow the existing code style in the repo
- Agent (C/ASM): Windows-only, cross-compiled with mingw

## License

By contributing to Mugen, you agree that your contributions will be licensed under GPL-3.0.
