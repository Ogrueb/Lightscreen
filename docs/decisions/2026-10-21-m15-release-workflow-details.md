# M15: details of the release workflow the plan left open

- **The dry run is a draft pre-release under a `dry-run-<run number>` tag.** A draft does not create its tag until it is published, and `dry-run-*` does not match `v*`, so publishing one by mistake would not start a release run; marking it a pre-release keeps it from ever being GitHub's "latest", which is what the update check asks for. Rejected: a dry run that only uploads an artifact (the plan's done-check asks for a draft release, and the release step is the part most likely to be wrong).
- **A tag must match the version in `CMakeLists.txt`**, compared after dropping trailing `.0` parts, as `QVersionNumber::normalized()` does in the update check. A tag that does not match would publish a release the update check offers forever, since the version compiled into the zip would always be older.
- **The release job runs the tests.** Same as CI; a tag on a commit whose CI was never green should not produce a package.
- **Two jobs: package (read-only token) and release (`contents: write`).** Only the job that creates the release gets write access, and it never runs for a pull request.
- **A pull request that touches `release.yml` runs the package job.** `workflow_dispatch` only works once a workflow is on the default branch, so without this the workflow could not be exercised before the merge.
- **The zip holds one top-level folder** named like the zip (`lightscreen-<version>-win64/`), so unzipping it does not spill the DLLs into the download folder. The CI artifact `lightscreen-win64` is unchanged.
