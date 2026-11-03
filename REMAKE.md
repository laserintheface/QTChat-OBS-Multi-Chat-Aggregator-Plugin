# QTChat: configurable YouTube reconnect wait

This update builds on the 250 ms stream-resume version. It preserves normal TLS/HTTP2 negotiation, resume tokens, duplicate filtering and bounded error retries, and adds a user-configurable pause after successful request completion.

## Install through your existing GitHub fork

1. Extract `QTChat-configurable-wait-update.zip`.
2. Open your existing QTChat GitHub repository and select the branch you build from.
3. Choose **Add file → Upload files**. Upload the **contents inside `repository-files`**, including `.github`, `src`, `tests` and the root files, to the repository root. Do not upload the enclosing folder or ZIP. This is an overlay for the existing repository: **do not delete original files** such as `cmake/`, `src/plugin-main.h`, `src/rtmp-detector.*`, `src/pch.h`, or your README/privacy policy. They are still needed.
4. Commit the upload. Your existing Push workflow runs tests and builds Windows. After a green run, download its Windows artifact. Create Release being skipped on a branch is expected.
5. Extract the artifact. When finished streaming, **close OBS**, back up the current QTChat DLL, and install the new plugin using the same location as your working installation. Reopen OBS.

This ZIP contains source, not a prebuilt Windows DLL. Keep the working Qt TLS backend; do not replace OBS's Qt libraries with different versions.

## Set your delay

Open **QTChat → Settings → YouTube → Reconnect wait after completion**.

| Setting | Meaning |
|---|---|
| 0.25 s | Original 250 ms behavior; default for existing users |
| 10 s | About a 20-second cycle if requests last ten seconds |
| 20 s | About a 30-second cycle |
| 35 s | About 9,600 chat units per 24 hours under the ten-second assumption |

Click **Save**. An open chat request is not interrupted. If already waiting after success, a changed value restarts that wait from Save. The reconnect button still initiates a manual attempt; errors retain their separate backoff. No automatic quota meter or budget cutoff is added.

For an INI edit, close OBS first. The setting is `youtube_reconnect_delay_ms=35000` for 35 seconds. Missing or malformed values preserve 250 ms; values are limited to 250–300,000 ms. The settings location remains `~/.config/obs-studio/qtchat-settings.ini`, including on Windows under the user profile.

The Settings dialog includes a **Quota guide and 24-hour example** button. For the full table, equations, reserves and assumptions, see [QUOTA-GUIDE.md](QUOTA-GUIDE.md).

## Default quota and the practical tradeoff

Google currently publishes **10,000 shared units per project per day**, not 15,000. The actual project allowance can differ; check the console. Each `streamList` opening/resume costs 5 units. Search has a separate daily allowance. Midnight Pacific Time is the quota reset.

Your tests reproduced approximately ten-second successful endings with both Qt and Python and both quiet and active chat. That duration is an observation, not a guaranteed API lifetime. Longer waits reduce requests but leave gaps when chat is not connected; new messages can arrive in batches on resume. Google controls continuation/history availability.

The ideal 24-hour calculation gives a 33.2-second extra wait with the entire 10,000 units available, which leaves no room for lookups. With a 100-unit reserve, it is about 33.64 seconds. The 35-second example leaves roughly 400 units of nominal headroom. It cannot guarantee a quota increase is unnecessary: shorter requests, prior use, retries and other applications can consume more. The guide also describes a more conservative 45-second example.

Official reference, checked November 3, 2026: https://developers.google.com/youtube/data/determine_quota_cost

## Data handling

Connections remain direct to Google and Twitch. YouTube credentials remain user-supplied. Chat rows remain in memory, capped at 500 per view, with up to 5,000 recent YouTube message IDs for deduplication. Connection diagnostics go to the OBS log; this update does not add a chat transcript, relay or analytics service.

The extra reconnect-delay preference is saved locally with the existing settings. Windows protects newly saved YouTube keys with DPAPI; non-Windows builds retain plaintext key storage. Older plaintext keys are migrated on Save. A software update does not remove prior backups or logs.

## Validation

Transport regression tests cover default and configurable successful waits, pending-wait changes, preserving continuation/deduplication state, not interrupting open replies, maintaining error backoff, stopping on quota errors, malformed frames and a local HTTP/2 response. Settings tests cover persistence, legacy defaults, malformed/out-of-range values and Cancel behavior, using an isolated temporary home directory.

Local validation status for this revision is recorded in `VALIDATION.md`. Windows linking/DPAPI behavior and the new setting in a live OBS session still require the GitHub build and your Windows test. No user API key is used by the local tests.

To run tests with CMake, Qt 6 and development tools available:

```sh
cmake -S tests -B test-build
cmake --build test-build --parallel 2
ctest --test-dir test-build --output-on-failure
```

In a complete source checkout, tests also compile-check all plugin translation units when OBS development headers are available. The overlay alone omits unchanged upstream files, so that full compile check is skipped until it is applied to the full repository.
