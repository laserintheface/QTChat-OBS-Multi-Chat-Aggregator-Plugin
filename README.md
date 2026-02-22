# QTChat - OBS Multi-Chat Aggregator Plugin

QTChat is an OBS Studio plugin that aggregates Twitch and YouTube chat into a single dockable window, with automatic detection of multiple RTMP streaming endpoints.

## Features

- **Multi-Platform Support**: Connect to Twitch and YouTube chat simultaneously
- **Auto-Detection**: Automatically detects which platforms you're streaming to
- **Dockable Window**: Fully dockable chat window in OBS
- **Combined View**: See all chats in one unified view, or separate tabs per platform
- **Easy Setup**: Simple settings dialog for OAuth configuration

## Quick Installation

### Option 1: Windows Installer (Recommended)

Download the latest installer from the [Releases](https://github.com/qtchat/obs-plugin/releases) page:

```
QTChat-Setup-1.0.0-x64.exe
```

Run the installer and follow the prompts:

1. Welcome screen - Click Next
2. License Agreement - Accept and click Next
3. Install Location - Default is recommended, click Next
4. Click Install
5. Click Finish to launch OBS

### Option 2: Manual Installation

1. Download `QTChat.dll` from the [Releases](https://github.com/qtchat/obs-plugin/releases) page
2. Copy to your OBS plugins folder:
   - `C:\Program Files\obs-studio\obs-plugins\64bit\`
3. Or for portable mode:
   - `C:\Program Files\obs-studio\portable\obs-plugins\`

## Requirements

- OBS Studio 30.0 or later
- Windows 10/11 (64-bit)

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

## Building from Source

### Prerequisites

1. Visual Studio 2022 with C++ development tools
2. CMake 3.16 or later
3. Inno Setup 6.x (for installer)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/qtchat/obs-plugin.git
cd obs-plugin

# Create build directory
mkdir build
cd build

# Configure with OBS Studio path
cmake -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_SYSTEM_PROCESSOR=AMD64 ^
  -DOBS_OUTPUT_DIR="rundir" ..

# Build
cmake --build . --config Release

# Create installer
cd ..
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\installer.iss
```

The installer will be created in `dist\QTChat-Setup-1.0.0-x64.exe`

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

### Project Structure

```
QTChat/
├── src/
│   ├── plugin-main.cpp      # Plugin entry point
│   ├── chat-dock.cpp        # Dockable chat UI
│   ├── chat-manager.cpp     # Chat connection management
│   ├── chat-message.cpp     # Message data structures
│   ├── twitch-irc.cpp       # Twitch IRC client
│   ├── youtube-chat.cpp     # YouTube API client
│   ├── rtmp-detector.cpp    # RTMP output detection
│   └── settings-dialog.cpp  # Settings configuration UI
├── installer/
│   └── installer.iss        # Inno Setup script
├── .github/
│   └── workflows/
│       └── build.yml        # CI/CD build workflow
└── CMakeLists.txt           # Build configuration
```

### Automated Builds

The project uses GitHub Actions for automated builds:

1. On push to `main`: Builds the plugin
2. On new tag `v*`: Creates a release with installer

## License

GPL-2.0 License - See LICENSE.txt

## Acknowledgments

- [OBS Studio](https://obsproject.com) - Broadcasting software
- [Twitch IRC Protocol](https://dev.twitch.tv/docs/irc) - Chat connectivity
- [YouTube Data API](https://developers.google.com/youtube/v3) - Live chat integration

## Support

- Report issues: https://github.com/qtchat/obs-plugin/issues
- Discussions: https://github.com/qtchat/obs-plugin/discussions
