#include <cstdarg>
#include <cstdio>
#include <QStringList>
QStringList diagnosticLog;
void blog(int, const char *format, ...) {
    char buffer[8192];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    const auto line = QString::fromUtf8(buffer);
    if (line.startsWith("[YouTubeDiag]")) diagnosticLog.append(line);
}
