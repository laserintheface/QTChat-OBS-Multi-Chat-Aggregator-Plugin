# Validation: configurable reconnect wait

Checked November 3, 2026.

- Built the transport tests and settings tests against real Qt 6.4.2 on Linux.
- CTest: both suites passed (2/2, 0 failures).
- Transport tests include the original 250 ms default, changed successful wait, restart of a pending wait without an extra request, resume-token preservation, unchanged error backoff, and not interrupting open replies. Existing framing, deduplication, quota-stop, retry-cap and local HTTP/2 tests also passed.
- Settings tests passed for legacy defaults, saving/reloading values, Cancel restoring the saved value, millisecond precision, invalid input and range limiting. The tests use an isolated temporary home directory and no real API key.
- Applied the source overlay to an isolated checkout of the existing GitHub repository. All plugin translation units compiled with real Qt and OBS development headers.
- Rendered the Settings dialog and static quota-guide dialog offscreen and checked their layout.
- Independently checked the table arithmetic, 24-hour equation and whole-call estimate: 1,920 requests at a 45-second cycle cost 9,600 chat units, plus one shared unit for the initial video lookup.

The local toolchain needed an explicit GNU C++ relink of the settings-test executable after its first link produced an empty output. The resulting executable passed both directly and through CTest. This was an environment/toolchain issue; no product-code workaround is included.

Not performed: Windows DLL linking, Windows DPAPI execution, installation in OBS on Windows, or an authenticated live test of this new setting. GitHub Actions must build the Windows artifact; verify that 10/20/35-second settings produce corresponding resume waits in your OBS log, and check actual Google quota usage. Existing user-run tests established the approximately ten-second API request endings; they did not validate this newly configurable build.

This is a source overlay for the existing repository. Unchanged bootstrap files and original headers are intentionally not included in the overlay ZIP.
