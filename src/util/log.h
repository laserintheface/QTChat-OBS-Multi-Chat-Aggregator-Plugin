#pragma once

// Log level constants - used with OBS's blog() function
// OBS's blog() accepts log levels: 0-10 (0=most verbose, 10=errors only)
#define LOG_DEBUG 4
#define LOG_INFO  3
#define LOG_WARN  2
#define LOG_ERROR 1
#define LOG_TRACE 5

// Convenience macros for OBS's blog() function (provided by obs-module.h)
#define blog_debug(...) blog(LOG_DEBUG, __VA_ARGS__)
#define blog_info(...)  blog(LOG_INFO, __VA_ARGS__)
#define blog_warn(...)  blog(LOG_WARN, __VA_ARGS__)
#define blog_error(...) blog(LOG_ERROR, __VA_ARGS__)
// Note: blog() is OBS's built-in function from <obs-module.h>
