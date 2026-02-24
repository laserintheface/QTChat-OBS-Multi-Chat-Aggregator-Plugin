#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>
#include <map>

#ifdef _WIN32
#define __attribute__(x)
#endif

#define LOG_DEBUG 4
#define LOG_INFO  3
#define LOG_WARN  2
#define LOG_ERROR 1
#define LOG_TRACE 5

// Stub implementation for obs_module_get_string
// This function is normally provided by OBS at runtime
static std::map<std::string, std::string> obs_module_string_map = {
    {"Plugin.Name", "QTChat"},
    {"Settings.Title", "Settings"},
    {"Settings.Twitch", "Twitch"},
    {"Settings.Enabled", "Enabled"},
    {"Settings.TwitchOAuth", "OAuth Token:"},
    {"Settings.TwitchChannel", "Channel:"},
    {"Settings.YouTube", "YouTube"},
    {"Settings.YouTubeAPIKey", "API Key:"},
    {"Settings.YouTubeChannelID", "Channel ID:"},
    {"Settings.Save", "Save"},
    {"Settings.Cancel", "Cancel"},
    {"Settings.Disconnected", "Disconnected"},
    {"Chat.Combined", "Combined"},
    {"Chat.Twitch", "Twitch"},
    {"Chat.YouTube", "YouTube"}
};

static const char* obs_module_get_string(const char* key)
{
    auto it = obs_module_string_map.find(key);
    if (it != obs_module_string_map.end()) {
        return it->second.c_str();
    }
    return key;
}

#ifndef obs_module_text
#define obs_module_text(key) obs_module_get_string(key)
#endif

#ifndef blog

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__APPLE__)
#define blog(level, ...) \
    do { \
        if (level <= 4) { \
            fprintf(stderr, "[%s] ", \
                (level) == LOG_DEBUG ? "debug" : \
                (level) == LOG_INFO ? "info" : \
                (level) == LOG_WARN ? "warning" : \
                (level) == LOG_ERROR ? "error" : "unknown"); \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n"); \
        } \
    } while(0)
#else
#define blog(level, ...) \
    __attribute__((format(printf, 2, 3))) \
    static inline void blog(level, const char *format, ...) \
    { \
        va_list args; \
        va_start(args, format); \
        if (level <= 4) { \
            fprintf(stderr, "[%s] ", \
                (level) == LOG_DEBUG ? "debug" : \
                (level) == LOG_INFO ? "info" : \
                (level) == LOG_WARN ? "warning" : \
                (level) == LOG_ERROR ? "error" : "unknown"); \
            vfprintf(stderr, format, args); \
            fprintf(stderr, "\n"); \
        } \
        va_end(args); \
    }
#endif

#ifdef __cplusplus
}
#endif

#endif

#define blog_debug(...) blog(LOG_DEBUG, __VA_ARGS__)
#define blog_info(...)  blog(LOG_INFO, __VA_ARGS__)
#define blog_warn(...)  blog(LOG_WARN, __VA_ARGS__)
#define blog_error(...) blog(LOG_ERROR, __VA_ARGS__)
