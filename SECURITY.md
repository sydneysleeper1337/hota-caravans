# Security policy

This experimental mod reads and writes memory inside the supported Heroes III process and uses a proxy-DLL loading technique. Antivirus products may therefore flag it heuristically. Reviewers can build the DLL directly from `src/` using `build.ps1`.

The project contains no network client, telemetry, self-updater or code-download mechanism. The installer operates only in the parent game directory, validates the supported executable hashes, creates separate launcher copies, and does not overwrite the original executables.

Please report suspected vulnerabilities privately to the repository owner through GitHub's private vulnerability-reporting feature when it is available. Do not include personal data or copyrighted game files in reports.

