#include <gtest/gtest.h>
#include <ith/ith_utility.h>

TEST(ithMemset16Test, DoNotSetAnythingIfInputSizeIsZero) {
    uint8_t buf[16];
    uint8_t expected[16];

    memset(buf, 0xFFU, sizeof(buf));
    memset(expected, 0xFFU, sizeof(buf));

    ithMemset16(&buf[2], 0U, 0U);

    EXPECT_EQ(memcmp(buf, expected, sizeof(buf)), 0);
}

TEST(ithMemset16Test, SetTwoBytesIfInputSizeIsOne) {
    uint8_t buf[16];
    uint8_t expected[16];

    memset(buf, 0xFFU, sizeof(buf));
    memset(expected, 0xFFU, sizeof(expected));
    expected[2] = 0U;
    expected[3] = 0U;

    ithMemset16(&buf[2], 0, 1U);

    EXPECT_EQ(memcmp(buf, expected, sizeof(buf)), 0);
}
