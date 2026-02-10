#pragma once

#include "clipboard.hh"

#if defined(WIN32)
#include "src/clipboardWIN32.cc"
std::string ClipLib::GetClipboard() {
    return ClipLibWin::GetClipboardText();
}

bool ClipLib::SetClipboard(std::string& str) {
    return ClipLibWin::SetClipboardText(str);
}

bool ClipLib::FormatAvailable(ClipLib::Format reqFormat) {
    auto clipboardFormat;
    switch (reqFormat) {
        case ClipLib::Format::TEXT_UTF8:
        case ClipLib::Format::TEXT_UTF16:
            clipboardFormat = CF_TEXT;
    }
}


#elif defined(__linux__)
#include "clipboardX11.cc"

std::string ClipLib::GetClipboard() {
    return ClipLibX11::GetSelectionAsUtf8();
}

bool ClipLib::SetClipboard(std::string& str) {
    return ClipLibX11::SetClipboardText(str.data());
}

bool ClipLib::FormatAvailable(ClipLib::Format reqFormat) {
    if (reqFormat == ClipLib::Format::UNKNOWN) return false;
    std::vector<Atom> targets;
    if (!ClipLibX11::GetClipboardTargets(targets)) return false;
    switch(reqFormat) {
        case ClipLib::TEXT_UTF8:
            return ClipLibX11::HasTarget(ClipLibX11::Utf8Atom, targets);
            break;
        case ClipLib::TEXT_UTF16:
            return ClipLibX11::HasTarget(XInternAtom(ClipLibX11::PrimaryDisplay, "UT16_STRING", False), targets);
            break;
        case ClipLib::IMAGE_PNG:
            return ClipLibX11::HasTarget(XInternAtom(ClipLibX11::PrimaryDisplay, "image/png", False), targets);
            break;
        default:
            return false;
    }

}

#endif