# QTChat Terms of Service

Last updated: November 3, 2026

## Scope

These terms describe the third-party service integrations in the QTChat OBS plugin fork maintained by **laserintheface**, available at [the project's GitHub repository](https://github.com/laserintheface/QTChat-OBS-Multi-Chat-Aggregator-Plugin). This version runs locally on your computer and receives chat from configured Twitch and YouTube channels. These terms do not describe independent forks or other plugins.

The maintainer does not operate a hosted chat service or provide QTChat user accounts, shared API credentials, or a shared YouTube quota allowance.

## Open-source software license

QTChat is distributed under the GNU General Public License included in [LICENSE.txt](LICENSE.txt), which contains GPL version 2. The applicable license notices in the source and any separately licensed components also apply.

These service integration terms do not replace that license or add restrictions to your rights to run, study, copy, modify, or redistribute the software under its applicable license. You do not need to accept a separate agreement with the maintainer merely to exercise those software rights. If these terms conflict with those rights, the applicable software license controls.

## YouTube and Twitch services

QTChat's YouTube integration uses YouTube API Services. **By using QTChat's YouTube features, you agree to be bound by the [YouTube Terms of Service](https://www.youtube.com/t/terms).** Your use of the API must also comply with the applicable [YouTube API Services Terms of Service](https://developers.google.com/youtube/terms/api-services-terms-of-service) and [Developer Policies](https://developers.google.com/youtube/terms/developer-policies).

If you enable Twitch, your use of Twitch's services is subject to the [Twitch Terms of Service](https://www.twitch.tv/p/en/legal/terms-of-service/).

These providers control access to their services. QTChat does not grant permission to use their services, content, or trademarks beyond permissions they provide. This fork is independently maintained and is not affiliated with or endorsed by YouTube, Google, Twitch, or the OBS Project.

## Your setup, credentials, and quota

To use YouTube, you must complete your own setup, including configuring a Google Cloud project, enabling the YouTube Data API, and supplying an API key you are authorized to use. This distribution does not include the maintainer's key. Requests made using your configured key consume the quota associated with its Google Cloud project; different keys in the same project do not create separate project allowances.

You are responsible for protecting your credentials and managing your project, access restrictions, and quota. Do not publish API keys in source code, screenshots, logs, or support requests. If a key is exposed, revoke or replace it through Google Cloud.

Google determines quota allowances, request costs, resets, and eligibility for increases. QTChat does not promise a particular number of streaming hours or approval of a quota increase. Reconnection settings can trade message delivery delay for fewer requests, but do not eliminate quota consumption. Use the services within their applicable rules; these settings are not permission to evade provider limits.

## Privacy and chat content

The [QTChat Privacy Policy](PRIVACY.md) explains the information this version processes, local settings and logs, and how to clear data or remove credentials. Google's handling of information is described in the [Google Privacy Policy](https://policies.google.com/privacy); Twitch's practices are described in the [Twitch Privacy Notice](https://www.twitch.tv/p/en/legal/privacy-notice/).

You remain responsible for how you display or share chat through OBS, recordings, screenshots, broadcasts, or other tools, including any applicable privacy and content requirements. Receiving a message through QTChat does not transfer ownership of that message or grant additional rights to reuse it.

## Availability, warranty, and support

Chat delivery depends on your computer, connection, OBS, and third-party services. Messages may be delayed, duplicated, missed, or unavailable. Providers may change or discontinue interfaces, restrict access, or interrupt connections. QTChat is not an archival service, and continued compatibility, updates, or support are not guaranteed.

The software's warranty exclusions and limitations of liability are those in its applicable license. Nothing in these terms excludes rights or remedies that applicable law does not allow to be excluded.

## Updates and contact

Future versions may change integrations or behavior. Material changes to these terms will be reflected in this document and its updated date. Updates do not reduce rights granted under the applicable software license.

For questions about these terms or this fork, use the maintainer contact listed in the [Privacy Policy](PRIVACY.md). For provider accounts, credentials, or quota decisions, contact the relevant provider.
