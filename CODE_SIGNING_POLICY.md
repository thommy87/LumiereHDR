# Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org).

## What is signed

Only binaries built from this repository's source code by the project's
GitHub Actions workflow (`.github/workflows/release.yml`) are signed:
`lumierehdr.exe`, `lumierehdr-cli.exe` and the Windows installer.
Third-party libraries shipped alongside them (Qt, Boost, LibRaw, OpenEXR
and others) are included unsigned and keep their own licenses.

LumièreHDR is a fork of [Luminance HDR](https://github.com/LuminanceHDR/LuminanceHDR).
Every change, including code taken from upstream, goes through the same
review before it is merged into the release branch.

## Team roles

| Role | Members |
| --- | --- |
| Authors (may commit without further review) | [thommy87](https://github.com/thommy87) |
| Reviewers (review changes from other contributors) | [thommy87](https://github.com/thommy87) |
| Approvers (approve every signing request) | [thommy87](https://github.com/thommy87) |

All team members use multi-factor authentication for GitHub and SignPath.

## Privacy

See [PRIVACY.md](PRIVACY.md). In short: this program will not transfer any
information to other networked systems unless specifically requested by
the user or the person installing or operating it.
