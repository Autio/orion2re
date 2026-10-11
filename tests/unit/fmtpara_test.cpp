#include "pch.h"

#include <cstring>

#include "doctest/doctest.h"
#include "game/fmtpara.h"

namespace test_fmtpara_runtime {
    void Reset_();
    const char* Printed_Text_();
}

DOCTEST_TEST_CASE("capture report inserts bound numbers instead of paragraph indices") {
    test_fmtpara_runtime::Reset_();
    FMTPARA::Init();

    int16_t values[8] = {3, 3, 0, 20, 4, 16, 54, 76};
    for (uint16_t i = 0; i < 8; ++i) {
        FMTPARA::Set_Paragraph_Sint_Pointer(i, &values[i]);
    }

    const char* report =
        "\0322Attacker Bonus: \030B6\aX175.Defender Bonus: \030B7\n"
        "\0321\030B0\aX80.\0321\030B3\aX255.\0320\n"
        "\0321\030B1\aX80.\0320\aX82. Killed"
        "\0321\030B4\aX255.\0320\aX257. Killed"
        "\n\aX48.-------\aX223.-------\n"
        "\0321\030B2\aX80.\0321\030B5\aX255.\0320\n\n\0322"
        "All Attacker Eliminated.";

    FMTPARA::Get_Formatted_Paragraph_Max_Height(339, report);
    FMTPARA::Print_Formatted_Paragraph(0, 0, 339, report, 0);

    DOCTEST_CHECK(std::strcmp(
        test_fmtpara_runtime::Printed_Text_(),
        "Attacker Bonus: 54Defender Bonus: 76"
        "3203 Killed4 Killed--------------016All Attacker Eliminated.") == 0);
}

DOCTEST_TEST_CASE("paragraph case modes preserve letters already in the requested case") {
    test_fmtpara_runtime::Reset_();
    FMTPARA::Init();

    FMTPARA::insert_case = 1;
    FMTPARA::Print_Formatted_Paragraph(0, 0, 339, "Ab", 0);
    DOCTEST_CHECK(std::strcmp(test_fmtpara_runtime::Printed_Text_(), "AB") == 0);

    test_fmtpara_runtime::Reset_();
    FMTPARA::insert_case = 2;
    FMTPARA::Print_Formatted_Paragraph(0, 0, 339, "aB", 0);
    DOCTEST_CHECK(std::strcmp(test_fmtpara_runtime::Printed_Text_(), "ab") == 0);

    test_fmtpara_runtime::Reset_();
    FMTPARA::insert_case = 3;
    FMTPARA::Print_Formatted_Paragraph(0, 0, 339, "Ab", 0);
    DOCTEST_CHECK(std::strcmp(test_fmtpara_runtime::Printed_Text_(), "Ab") == 0);
}