# QTChat Privacy Policy

Last updated: November 3, 2026

## Scope and maintainer

This policy describes the QTChat OBS plugin fork maintained by **laserintheface**, distributed through its public GitHub repository, specifically the version that connects directly to Twitch and to YouTube API Services using a user-supplied YouTube API key. It does not describe the upstream project's other versions, independently modified forks, or other OBS plugins.

QTChat runs on the user's computer. This fork was developed for the maintainer's own streaming setup. Other people who choose to use it must install the plugin and complete their own service setup, including creating or configuring their own Google Cloud project, enabling the YouTube Data API, and supplying their own API key. This distribution does not supply the maintainer's API key, a shared quota allowance, a hosted chat service, or a QTChat user account. Google controls API availability, quotas, and any approval for quota increases.

Privacy contact: lazerintheface@gmail.com

## Information accessed and its purpose

QTChat uses information only to connect to the configured channels, receive and display chat in OBS, maintain the connection, prevent duplicate messages, save preferences, and diagnose connection problems.

The plugin processes:

- **Settings you enter:** your Twitch channel, YouTube channel ID, optional broadcast URL or video ID, YouTube API key, and platform enable/disable preferences.
- **Chat and broadcast data:** public broadcast and live-chat identifiers; message IDs, display names, message text, timestamps, and available account-role information such as broadcaster, moderator, or membership/subscriber status. Platform responses may contain additional fields even when QTChat does not display them.
- **Temporary connection state:** YouTube continuation tokens and recent message IDs, used to resume chat and avoid displaying repeated messages.
- **Diagnostics:** connection status, error codes, channel identifiers, and settings-file paths written to the OBS log.

This version reads YouTube public live-chat data using an API key. It does not use Google account sign-in or request Google OAuth authorization. Twitch reception uses an anonymous chat connection. QTChat does not ask for your Google or Twitch password, send chat messages, or perform moderation actions.

## Local storage and retention

Settings are saved on your computer until you change or delete them. The reviewed version stores them in `qtchat-settings.ini` under `.config/obs-studio` in your user home directory, including on Windows. A typical Windows path is `%USERPROFILE%\.config\obs-studio\qtchat-settings.ini`.

On Windows, newly saved YouTube API keys are protected using Windows Data Protection API (DPAPI), associated with the Windows user account. Software running with your account's privileges may still access the key. Existing keys from older versions may remain plaintext until you save settings with this version; previous backups are not automatically removed. Non-Windows builds store the key as plaintext in the settings file.

Chat rows are held in memory, with a default limit of 500 rows per view. Older rows are removed when that limit is exceeded. The plugin does not create a persistent chat transcript. Clicking **Clear** removes the displayed rows. YouTube duplicate filtering retains up to 5,000 recent message IDs in memory; these and continuation tokens may remain after the displayed rows are cleared, until a new YouTube session is started or OBS exits.

OBS logs can remain on disk after a session ends. Their retention is controlled by OBS and your handling of those files, not by QTChat. The reviewed plugin does not intentionally log chat message bodies or API keys; YouTube status text includes key-redaction safeguards. Logs can still identify your channels and local user path. Other software, operating-system facilities, backups, recordings, or modified plugin builds may retain additional information.

## Where information goes

QTChat connects directly to Google/YouTube and Twitch over encrypted connections. Your YouTube API key and lookup/chat parameters are sent to Google as needed for API requests. Your selected Twitch channel is sent to Twitch. These providers also receive ordinary connection information, such as your IP address, and handle requests under their own policies.

The plugin does not send chat messages, settings, or API keys to a server operated by this fork's maintainer. It contains no added analytics, advertising, tracking service, or automatic diagnostic-upload feature. It does not sell chat or settings data. Merely using the plugin does not give the maintainer access to those local records.

If you voluntarily submit logs, screenshots, recordings, or support messages, their contents are disclosed to the recipient. Public issue trackers and public recordings can be viewed by others. Remove credentials and any information you do not want to share before submitting material. Any third-party site hosting this policy, downloads, or support discussions has its own privacy practices.

If you capture the chat dock in a stream or recording, you control that separate publication and any resulting recording. Clearing QTChat does not remove content from those recordings or from the original platform.

## Your controls and deletion

You can:

1. Disable a platform in QTChat Settings and save to stop that platform's connection, or close OBS to stop all plugin activity.
2. Click **Clear** to remove displayed chat rows. Close OBS to release the plugin's remaining session data from normal application memory.
3. Remove the API key and channel/video settings and save to replace the stored settings. For complete removal of the plugin's settings file, close OBS and delete `qtchat-settings.ini` from the location above. Uninstalling the plugin alone may leave this file behind.
4. Delete OBS log files and any backups, screenshots, recordings, or exported copies you control if you want to remove information in those files. Clearing the dock or deleting settings does not delete logs.
5. Revoke an API key by deleting it, or replace it with a new key, in Google Cloud Console. Removing a key from QTChat does not revoke the key at Google.

Deletion on your computer does not delete the original messages from YouTube or Twitch or information retained by those providers. Use the provider's controls for that data. The maintainer cannot remotely delete data on other users' computers. For questions about information you separately supplied to the maintainer, use the privacy contact above; do not send passwords or API keys.

This version does not obtain Google OAuth access. If you separately authorized another application, its Google account access can be reviewed at [Google's third-party connections page](https://myaccount.google.com/connections); this is separate from deleting an API key.

## Platform policies and terms

QTChat uses **YouTube API Services**. By using QTChat's YouTube features, you agree to be bound by the [YouTube Terms of Service](https://www.youtube.com/t/terms).

- [Google Privacy Policy](https://policies.google.com/privacy)
- [Twitch Privacy Notice](https://www.twitch.tv/p/en/legal/privacy-notice/)
- [Twitch Terms of Service](https://www.twitch.tv/p/en/legal/terms-of-service/)

## Changes

The maintainer may update this policy when the plugin's data handling changes. Changes will be published at this policy's public URL with an updated date. A version that adds hosted services, different credentials, browser capture, analytics, or other data processing needs an updated policy reflecting that implementation.
