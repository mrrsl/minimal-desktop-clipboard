#pragma once

#include <string>

namespace ClipLib {
    enum Format {
        TEXT_UTF8,
        TEXT_UTF16,
        IMAGE_PNG,
        UNKNOWN
    };
    std::string GetClipboard();
    bool SetClipboard(std::string&);
    bool FormatAvailable(Format);
}

#ifdef HEADER_ONLY
#include "../src/clipboard.cc"
#endif
