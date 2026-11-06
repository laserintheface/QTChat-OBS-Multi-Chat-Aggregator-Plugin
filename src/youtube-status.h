#pragma once
#include <string_view>

namespace YouTubeStatus {
inline bool terminal(int code) {
    return code == 3 || code == 5 || code == 7 || code == 8 || code == 9 || code == 16;
}
inline int errorPriority(int code) {
    // NOT_FOUND may be refreshed, but must never erase quota/auth stop codes.
    return terminal(code) && code != 5 ? 3 : (code == 5 ? 2 : (code > 0 ? 1 : 0));
}
struct Parsed {
    bool present = false, valid = false, conflict = false;
    int code = -1, error = -1, values = 0;
};
// Qt can expose repeated field values as a comma-separated string. Only accept
// success when EVERY member is a valid code and all members agree. Never log raw.
inline Parsed parse(std::string_view raw, bool present) {
    Parsed out; out.present = present;
    if (!present || raw.size() > 1024) return out;
    bool malformed = false;
    while (true) {
        if (++out.values > 64) { malformed = true; break; }
        const auto comma = raw.find(',');
        auto part = raw.substr(0, comma);
        while (!part.empty() && (part.front() == ' ' || part.front() == '\t')) part.remove_prefix(1);
        while (!part.empty() && (part.back() == ' ' || part.back() == '\t')) part.remove_suffix(1);
        int code = 0;
        bool numeric = !part.empty() && part.size() <= 2;
        for (char c : part) {
            if (!numeric) break;
            if (c < '0' || c > '9') { numeric = false; break; }
            code = code * 10 + c - '0';
        }
        numeric = numeric && code <= 16;
        if (!numeric) malformed = true;
        else {
            if (out.code < 0) out.code = code;
            else if (out.code != code) out.conflict = true;
            if (code != 0 && (out.error < 0 || errorPriority(code) > errorPriority(out.error))) out.error = code;
        }
        if (comma == std::string_view::npos) break;
        raw.remove_prefix(comma + 1);
    }
    out.valid = !malformed && !out.conflict && out.code >= 0;
    return out;
}
} // namespace YouTubeStatus
