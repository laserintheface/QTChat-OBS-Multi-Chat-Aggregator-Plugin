# QTChat 1.1 streaming remake

This source update replaces QTChat's repeating YouTube chat polling with the official `streamList` server stream. It keeps the native OBS dock and direct Twitch/YouTube connections. It is a preview that needs a Windows build and an authenticated live-broadcast test before relying on it for a full stream.

## Successful stream completion and resume update

Windows now negotiates HTTP/2 successfully and receives chat batches. In the supplied live log, each RPC completes after approximately ten seconds with HTTP 200, gRPC 0, and network error 0. This does not establish why the server completes each RPC. Google's demo starts another StreamList request using the last next_page_token after an RPC completes.

Previously QTChat treated every completion as a failure, causing 15/30/60/120/240-second gaps and eventually stopping. This revision renews clean completed RPCs after 250 ms using the saved token, preserves message deduplication and logical chat connection, and resets the failure count. Successful renewals no longer hit the old 100-request session limit. Genuine failures retain bounded exponential backoff. A completion must have valid data, a resume token, complete frames, and explicit successful gRPC/network/HTTP statuses to qualify. Empty or malformed responses do not create an immediate reconnect loop. Broadcast-ended and quota/authentication errors still stop.

The diagnostic network=0 now displays `none` rather than Qt's misleading default `Unknown error` text. The Qt caching warning has not been proven to cause these successful RPC completions; this revision does not claim to eliminate that warning.

Regression tests were added for repeated clean completions with duplicate history, disconnect cancellation, empty successful responses, and truncated frames. They have not been executed locally: CMake and Qt development packages are unavailable here. The included GitHub workflow runs the tests and Windows build. This revision still requires an authenticated Windows live test.

After installing, expect `Stream completed successfully; resuming chat in 250 ms` rather than increasing failure delays on gRPC 0. Send test messages over several minutes to verify new messages continue arriving without duplicates.

## HTTP/2 negotiation update

The previous Windows log found the broadcast and chat, then failed before receiving messages with an HTTP/2 protocol error. This revision disables forced HTTP/2 direct mode so Qt negotiates the protocol through TLS ALPN. It also logs TLS backend/ALPN, HTTP status, actual HTTP/2 use, gRPC status, and network error. It does not log request headers or chat bodies; status text uses the existing key redaction.

This is a candidate fix, not a confirmed resolution of the Windows error. No credentials were used. This revision could not be compiled locally because CMake and Qt development packages are unavailable. The included GitHub workflow runs tests before building Windows. Earlier validation results below describe the original streaming remake, not a new run of this revision.

After installing, reconnect YouTube and look for `Connected via streamList`. If it fails, copy the new `[YouTubeChat] Stream` diagnostic lines from the OBS log. A reused TLS connection may omit the TLS-ready signal; the response/finished lines still report HTTP/2 use.

## Build using your existing GitHub fork

1. Extract `QTChat-stream-resume-update.zip`.
2. Open https://github.com/laserintheface/QTChat-OBS-Multi-Chat-Aggregator-Plugin and select your main branch.
3. Choose **Add file → Upload files**. Open the extracted `repository-files` folder and drag **its contents** into the upload area. Include the `.github`, `src`, and `tests` folders and the root files. Upload these at the repository root; do not upload the enclosing `repository-files` folder or the ZIP itself.
4. Commit the upload. The existing **Push** workflow runs the tests and Windows build. You can also choose **Actions → Push → Run workflow**.
5. Open the completed green run. Under **Artifacts**, download `QTChat-1.1.0-windows-x64-...`. An artifact is the downloadable output of the build. **Create Release** being skipped on an ordinary branch is expected.
6. Extract the artifact. When you are finished streaming, close OBS, back up your current QTChat DLL, and install the plugin files using the same location as your working installation. Reopen OBS. Never replace a DLL while OBS is running.

This update contains source files, not a prebuilt DLL. Keep your existing working Qt 6.11.1 TLS backend. Do not replace OBS's Qt DLLs or copy a TLS backend from a different Qt version.

## Connect chat

Open **QTChat → Settings**. Your existing Twitch channel and YouTube key/channel settings are read from the previous settings file.

- Twitch: enter your channel name and enable it. Receiving chat uses anonymous IRC over TLS; no Twitch OAuth token is required.
- YouTube: enter your API key and enable it. For the most predictable connection, paste the **current broadcast's video URL** into **Video URL or ID**. A video URL bypasses channel search. You can leave the channel ID blank when supplying a video URL.
- Save. Open the dock through **QTChat → Combined** if necessary.
- Start your YouTube broadcast with live chat enabled, then click **Reconnect YouTube** in the dock. You do not need to restart OBS. Replace an old video URL when you create a new broadcast.

If you leave the video field blank, QTChat searches once for a public live broadcast on the supplied `UC...` channel ID. If none exists, it stops and waits for you to reconnect. It does not continuously search while you are offline.

## What changed

- YouTube server streaming with resume tokens and duplicate filtering; no fallback to `liveChatMessages.list` polling.
- Separate connection status, useful errors, and a YouTube reconnect button.
- Quota/authentication errors stop automatically. Temporary failures back off from 15 seconds to five minutes, with eight automatic retries; successful stream completion resets that failure count. Manual reconnect starts a new session.
- Repeated RTMP output detection cannot silently restart a failed YouTube connection.
- Saving changed settings takes effect immediately. Saving unchanged active YouTube settings does not create another connection.
- Twitch uses encrypted IRC and preserves messages split across network reads. Tagged messages, hexadecimal name colors, moderator badges, and emote tags are handled.
- Scrolling up to read older messages no longer gets overridden by a repeating scroll timer.
- API keys travel in request headers and are redacted from error text. Windows saves the key using DPAPI protection tied to the Windows account. Existing plaintext settings migrate when you click Save.

## Privacy and limits

Connections go directly to Google and Twitch. There is no added relay, analytics service, account registration, or persistent chat transcript. Chat rows remain in memory, capped at 500 per view; recent YouTube message IDs are capped at 5,000 for duplicate filtering. OBS logs connection status and channel identifiers, but this implementation does not log API keys or message contents.

DPAPI protects the key at rest; software running as your Windows user can still access it. Non-Windows builds retain plaintext key storage. The legacy settings location remains `~/.config/obs-studio/qtchat-settings.ini` for compatibility. The old plugin version may not understand the protected key if you roll back; re-enter the key in that version if needed. Rotate any key previously shared in logs.

YouTube API quota still applies. Opening/reopening a stream and looking up broadcasts consume quota; clean RPC completion renews the stream rather than using the polling endpoint; this is not unlimited API access. A quiet chat keeps its existing connection open. Message volume does not drive a local polling loop. Actual quota use should be checked in your Google project during the live test. A quota-exhausted project must recover quota before this version can connect.

This version is read-only. It does not send messages, moderate chat, render third-party emotes, retrieve private/member-only broadcasts through OAuth, or remove previously displayed messages when moderation events occur. YouTube chat availability and the initial history returned are controlled by YouTube. The manual retry limit favors avoiding runaway requests over unattended reconnection forever. Twitch retries up to five times after failures; saving settings retries a disconnected Twitch connection.

## Validation and first live test

For the original streaming remake, all plugin translation units compiled against real Qt 6.4.2 and OBS development headers on Linux. Automated checks cover protobuf fields, fragmented/coalesced frames, Unicode, malformed frames, duplicate filtering, resume tokens, retry limits, stale replies, key redaction, URL validation, Twitch tags, and a real local HTTP/2 stream with quota trailers. An unauthenticated TLS/HTTP2 connection to Google's actual streamList service also succeeded and returned a gRPC permission response, as expected without credentials.

Windows linking, DPAPI execution, OBS dock behavior on Windows, and authenticated YouTube streaming have **not** been executed here. The included GitHub workflow compiles Windows after the Linux tests pass. No user credentials were used in testing.

After installing, verify Twitch and YouTube messages appear in Combined, scroll up and check that your position remains stable, and inspect Google quota usage over a short broadcast. Try Reconnect YouTube without restarting OBS. Keep the previous DLL for rollback.

## Development references

- Google streamList guide and protobuf schema: https://developers.google.com/youtube/data/live/streaming-live-chat
- streamList API reference: https://developers.google.com/youtube/v3/live/docs/liveChatMessages/streamList
- gRPC HTTP/2 wire protocol: https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md

`src/youtube-wire.h` implements the read-only subset of Google's published schema; Qt Network handles TLS and HTTP/2. The transport requests uncompressed gRPC messages and rejects malformed or oversized frames. It deliberately has no gRPC library dependency or polling fallback.

To run local tests with Qt 6 and OBS development packages installed:

```sh
cmake -S tests -B test-build
cmake --build test-build --parallel 2
ctest --test-dir test-build --output-on-failure
```
