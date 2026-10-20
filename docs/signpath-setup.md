# Setting up SignPath code signing

LumièreHDR's Windows binaries are signed for free by the
[SignPath Foundation](https://signpath.org) open-source program. The
release workflow (`.github/workflows/release.yml`) already contains the
signing step; it runs once the steps below are done.

## 1. Before applying

- Turn on two-factor authentication for every maintainer's GitHub account.
- Publish at least one (unsigned) release, because SignPath only signs
  projects that are already released: push a tag such as `v2.7.0` and
  publish the draft release the workflow creates.
- Keep [CODE_SIGNING_POLICY.md](../CODE_SIGNING_POLICY.md) and
  [PRIVACY.md](../PRIVACY.md) linked from the README and the release page.

## 2. Apply

Apply at <https://signpath.org/apply> with the repository URL
`https://github.com/thommy87/LumiereHDR`. Mention that it is a visible fork
of Luminance HDR and that releases are built only by GitHub Actions.

## 3. Configure SignPath once approved

Create these in the SignPath organization you are given:

| Item | Value |
| --- | --- |
| Project slug | `LumiereHDR` |
| Signing policies | `release-signing` (manual approval by an approver), `test-signing` |
| Trusted build system | GitHub.com, linked to this repository |
| Artifact configuration slug | `windows-zip` |

Artifact configuration `windows-zip` (the GitHub artifact is a ZIP that
contains the release ZIP):

```xml
<?xml version="1.0" encoding="utf-8"?>
<artifact-configuration xmlns="http://signpath.io/artifact-configuration/v1">
  <zip-file>
    <zip-file path="LumiereHDR-*-windows-x64.zip">
      <directory path="LumiereHDR-*-windows-x64">
        <pe-file path="luminance-hdr.exe" product-name="LumiereHDR">
          <authenticode-sign />
        </pe-file>
        <pe-file path="luminance-hdr-cli.exe" product-name="LumiereHDR">
          <authenticode-sign />
        </pe-file>
      </directory>
    </zip-file>
  </zip-file>
</artifact-configuration>
```

## 4. Connect GitHub

In the repository settings on GitHub:

- **Secrets > Actions:** `SIGNPATH_API_TOKEN`, an API token of a SignPath
  CI user with submitter rights on the project.
- **Variables > Actions:** `SIGNPATH_ORGANIZATION_ID`, your SignPath
  organization ID.

From then on, every pushed `v*` tag builds, waits for an approver to
approve the signing request in SignPath, and drafts a release with the
signed package.
