# YouTube chat delay and daily quota

Verified against Google's documentation on November 3, 2026.

## Default quota: 10,000, not 15,000

Google currently publishes a default allocation of **10,000 units per project per day** for the shared bucket containing live-chat methods. `search.list` has its own default allowance of 100 calls/day, separate from that shared bucket. `videos.insert` also has its own bucket and is not used by this plugin.

Your project's actual allowance may differ after an adjustment. Check **Google Cloud Console → APIs & Services → YouTube Data API v3 → Quotas & System Limits** for the selected project's current limits and usage. If your console says 15,000 for the relevant daily quota, use that number in the equations; it is not the published default. Quota resets at **midnight Pacific Time**, not on a rolling 24-hour timer and not at local midnight everywhere.

Google lists these costs:

| Request | Cost and purpose |
|---|---|
| `liveChatMessages.streamList` | **5 shared units each time a new chat request opens**, including resuming with a continuation token |
| `videos.list` | **1 shared unit** to find the broadcast's active live-chat ID |
| `search.list` | **1 call from the separate 100-search daily bucket**, used when finding a public live broadcast from a channel ID |

Use the current broadcast's video URL to bypass search. You still need the video lookup. Receiving more messages or more batches within one open chat request does not itself create more local API calls. The saved chat token is a bookmark, not a free-request credential. Other applications using the same project also consume its applicable quotas. Twitch requests do not consume YouTube quota.

Official reference: [YouTube quota calculator](https://developers.google.com/youtube/data/determine_quota_cost). See also [streamList](https://developers.google.com/youtube/data/live/reference/rest/v3/liveChatMessages/streamList) and [Google's streaming example](https://developers.google.com/youtube/data/live/streaming-live-chat).

## Change the wait without rebuilding

Open **QTChat → Settings → YouTube → Reconnect wait after completion**. Enter seconds and click **Save**.

- **0.25 s** preserves the original 250 ms build's behavior and is the default for existing settings files.
- **10 s** or **20 s** reduces requests at the cost of delayed chat.
- **35 s** is the example for approximately 24 hours within a full default daily allowance, with some headroom, assuming the measured ten-second request duration continues.

The allowed range is **0.25–300 seconds**. It is also saved as milliseconds in `qtchat-settings.ini`:

```ini
youtube_reconnect_delay_ms=35000
```

The INI location remains `~/.config/obs-studio/qtchat-settings.ini`, including on Windows (`%USERPROFILE%\.config\obs-studio\qtchat-settings.ini`). Close OBS before editing this file manually, then reopen OBS. A missing or malformed delay uses 250 ms. Numeric values outside the supported range are clamped.

Changing this setting while a chat request is open does **not** interrupt it or issue an extra lookup. The new delay is used when that request completes successfully. If a successful-completion wait is already pending, changing and saving the setting restarts that wait from the time of Save. Saving an unchanged delay does not restart it. This option does not shorten network-error backoff or restart a stopped quota-error session.

## What ten seconds means

Live tests of both QTChat and an independent Python gRPC client received successful request completions after about **10.4–10.7 seconds**, with both quiet and active chat. Ten seconds below is a rounded planning assumption, **not** a configurable timeout or a guaranteed server duration.

The sequence is: open a chat request → receive batches while it is open → successful request completion → wait the configured extra delay → open the next request using the saved token.

During the extra wait, no chat request is open. Messages can be returned in batches after reconnection, so longer waits make chat less immediate. Resume tokens and duplicate filtering are retained. YouTube controls how much history is available and whether continuation tokens remain usable; waiting is not a guarantee of complete archival capture.

## Useful numbers for one chat

These estimates assume ten-second requests, one active chat reader, a full quota allowance, and no failures. The first budget column uses all 10,000 units for chat and therefore slightly overstates practical runtime. The second reserves **100 units**, leaving **9,900** for chat. Times are approximate and rounded to the nearest minute.

| Extra reconnect wait | Total request cycle | Requests/hour | Chat units/hour | Runtime with 10,000 chat units | Runtime with 9,900 chat units |
|---|---|---:|---:|---|---|
| 0.25 s (default) | 10.25 s | 351.22 | 1,756.10 | 5 h 42 min | 5 h 38 min |
| 10 s | 20 s | 180 | 900 | 11 h 7 min | 11 h |
| 20 s | 30 s | 120 | 600 | 16 h 40 min | 16 h 30 min |
| 35 s | 45 s | 80 | 400 | 25 h | 24 h 45 min |

A **10-second total cycle** with no pause, for comparison, uses about **1,800 units/hour**, or about **5 h 33 min per 10,000 units**. The selectable default adds a 0.25-second pause. An extra 10-second wait therefore means a roughly **20-second total cycle**, not a ten-second cycle.

The 25-hour figure above is mathematical coverage at that rate, not a promise of a 25-hour daily allowance; daily resets, actual request lengths, previous usage, errors and other clients affect the result.

## Equations

Let:

- `L` = average open-request duration in seconds (use `10` for this example).
- `D` = configured extra wait in seconds.
- `Q` = units currently available for the intended quota window, after previous usage.
- `R` = units reserved for discovery, retries, other applications and margin.
- `B = Q - R` = usable chat budget; it must be positive.
- `H` = intended streaming time in hours.

```text
Total cycle in seconds = L + D
Requests per hour      ≈ 3600 / (L + D)
Quota units per hour   ≈ 5 × 3600 / (L + D)
                      = 18000 / (L + D)
Hours available        ≈ B × (L + D) / 18000
Required extra wait   ≈ max(0, 18000 × H / B - L)
```

Requests are charged as whole calls, not fractions. For a constant cycle, an initially opened request, and a session lasting `H` hours:

```text
Number of requests ≈ ceil(3600 × H / (L + D))
Chat quota         ≈ 5 × number of requests
Check: chat quota + other usage <= your actual quota allowance
```

The ceiling accounts for the request opened at the start. Real request durations vary, and genuine failures follow a different retry schedule. These are planning estimates, not an enforced quota meter or automatic cutoff.

## The 24-hour example

With `L = 10`, `H = 24` and the complete `10,000` units available:

```text
D ≈ 18000 × 24 / 10000 - 10 = 33.2 seconds
```

That is only the ideal mathematical boundary: approximately **2,000 chat requests × 5 = 10,000 units**, leaving nothing for even the initial video lookup. It is not a practical setting for a fresh project session.

With a **100-unit reserve**, the usable chat budget is `9,900`:

```text
D ≈ 18000 × 24 / 9900 - 10 ≈ 33.64 seconds
```

Round up to a **35-second extra wait** for this example:

```text
10 + 35 = 45 seconds per cycle
86400 / 45 = 1920 requests per 24 hours
1920 × 5 = 9600 chat units
10000 - 9600 = 400 units of nominal headroom
One video lookup brings the simple estimated total to 9601 units.
```

This can fit a full 24 hours of the observed behavior inside 10,000 units without a quota increase. **It is not guaranteed:** shorter successful requests, frequent errors, manual reconnects, other readers, or prior use reduce that headroom. For example, if requests started completing immediately, a 35-second wait alone would average roughly 12,343 units/24 hours. A **45-second wait** would keep the ideal steady-success estimate at or below 9,600 chat units/24 hours even with zero open-request duration; genuine error retries and other usage can still add cost.

Check actual quota usage after a short run and leave extra margin. The calculation treats the requested 24 hours as a single budget window; crossing Google's midnight-Pacific reset may split usage between daily windows.
