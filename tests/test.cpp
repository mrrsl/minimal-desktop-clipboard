#include <gtest/gtest.h>
#include <string>
#include "clipboard.hh"

void SimplePasteCheck(const char* text) {
    auto textString = std::string(text);

    bool status = ClipLib::SetClipboard(textString);
    ASSERT_EQ(status, true) << "SetClipboard() has failed";

    status = ClipLib::FormatAvailable(ClipLib::Format::TEXT_UTF8);
    EXPECT_EQ(status, true) << "Expected format not available";

    std::string clipboard = ClipLib::GetClipboard();
    EXPECT_EQ(clipboard, textString) << "Retrieved string and test string do not match";
}

TEST(InternationalCharacterTests, Japanese) {
    const char* text = "ユーザー別サイト";
    SimplePasteCheck(text);   
}

TEST(InternationalCharacterTests, Chinese) {
    const char* text = "简体中文";
    SimplePasteCheck(text);
}

TEST(InternationalCharacterTests, Korean) {
    const char* text = "크로스 플랫폼으로";
    SimplePasteCheck(text);
}

TEST(InternationalCharacterTests, MathSymbols) {
    const char* text = "∮ E⋅da = Q, n → ∞, ∑ f(i) = ∏ g(i)";
    SimplePasteCheck(text);
}

TEST(InternationalCharacterTests, Thai) {
    const char* text = "แผ่นดินฮั่นเสื่อมโทรมแสนสังเวช";
    SimplePasteCheck(text);
}
