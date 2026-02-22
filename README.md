# QTChat - OBS Multi-Chat Aggregator Plugin

QTChat is an OBS Studio plugin that aggregates Twitch and YouTube chat into a single dockable window, with automatic detection of multiple RTMP streaming endpoints.

## Features

- **Multi-Platform Support**: Connect to Twitch and YouTube chat simultaneously
- **Auto-Detection**: Automatically detects which platforms you're streaming to
- **Dockable Window**: Fully dockable chat window in OBS
- **Combined View**: See all chats in one unified view, or separate tabs per platform
- **Easy Setup**: Simple settings dialog for OAuth configuration

## Installation

### Quick Install

1. Download `QTChat.dll` from the [Releases](https://github.com/Signal-Six/QTChat-OBS-Multi-Chat-Aggregator-Plugin/releases) page
2. Copy the DLL to your OBS plugins folder:
   - Standard: `C:\Program Files\obs-studio\obs-plugins\64bit\`
   - Portable: `C:\Program Files\obs-studio\portable\obs-plugins\64bit\`
3. Restart OBS Studio

## Usage

1. Launch OBS Studio
2. Go to **Tools** menu → **QTChat Settings**
3. Configure your platform credentials:

### Twitch Setup
- **OAuth Token**: Get from [twitchapps.com/tmi](https://twitchapps.com/tmi/)
- **Channel Name**: Your Twitch channel name

### YouTube Setup
- **API Key**: Get from [Google Cloud Console](https://console.cloud.google.com/)
  - Enable YouTube Data API v3
  - Create credentials (API Key)
- **Channel ID**: Your YouTube channel ID (starts with UC...)

4. Click **Save**
5. The chat dock will appear in OBS

## Requirements

- OBS Studio 30.0 or later
- Windows 10/11 (64-bit)

## Building from Source

### Prerequisites

1. Visual Studio 2022 with C++ development tools
2. CMake 3.16 or later
3. OBS Studio 30.2 source code
4. OBS dependencies (prebuilt)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/Signal-Six/QTChat-OBS-Multi-Chat-Aggregator-Plugin.git
cd QTChat-OBS-Multi-Chat-Aggregator-Plugin

# Download OBS Studio source
curl -L -o obs-studio.zip https://github.com/obsproject/obs-studio/archive/refs/tags/30.2.2.zip
unzip obs-studio.zip
mv obs-studio-30.2.2 obs-studio

# Download OBS dependencies
curl -L -o obs-deps.zip https://github.com/obsproject/obs-deps/releases/download/2024-09-12/windows-deps-2024-09-12-x64.zip
unzip obs-deps.zip

# Build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DOBS_STUDIO_DIR=obs-studio ^
  -DOBS_DEPS_DIR=obs-deps

cmake --build build --config Release
```

The DLL will be in `build\rundir\obs-plugins\64bit\QTChat.dll`

## Configuration

When streaming to multiple platforms:

- The plugin automatically detects which RTMP outputs are active
- Chat messages are routed to the appropriate panels
- Use the tabbed view to see combined or platform-specific chats

### Chat Dock Controls

- **Combined Tab**: Shows all chat messages from connected platforms
- **Twitch Tab**: Shows only Twitch chat
- **YouTube Tab**: Shows only YouTube chat
- **Clear Button**: Clears all messages from view

## Troubleshooting

### Connection Issues
- Verify your OAuth token is correct (starts with `oauth:`)
- Ensure your YouTube API key has access to the Live Chat API
- Check OBS log for error messages (View → Logs → Current Log File)

### Platform Detection
- Make sure your streaming output is configured before starting the stream
- The plugin checks for active outputs every 5 seconds during streaming

### Plugin Not Loading
- Ensure OBS Studio is 64-bit
- Verify the DLL is in the correct folder: `obs-plugins\64bit\`
- Check OBS log for any load errors

## Development

The project uses GitHub Actions for automated builds. On push to main, the plugin is automatically built and tested.

## License

GPL-2.0 License

## Acknowledgments

- [OBS Studio](https://obsproject.com) - Broadcasting software
- [Twitch IRC Protocol](https://dev.twitch.tv/docs/irc) - Chat connectivity
- [YouTube Data API](https://developers.google.com/youtube/v3) - Live chat integration
