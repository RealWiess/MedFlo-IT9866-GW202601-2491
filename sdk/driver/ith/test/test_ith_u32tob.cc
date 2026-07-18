#include <gtest/gtest.h>
#include <ith/ith_utility.h>

void dump_hex(const char *buf, int size) {
    for (int i = 0; i < size; i++) {
        printf("%c ", buf[i]);
    }
    printf("\n");
}

TEST(ithU32tob, DoNotSetAnythingIfInputSizeIsZero) {
    char buf[42];
    char expected[42];

    memset(buf, 0xFFU, sizeof(buf));
    memset(expected, 0xFFU, sizeof(buf));

    // 1
    expected[0 + 4] = '0';
    expected[1 + 4] = '0';
    expected[2 + 4] = '0';
    expected[3 + 4] = '1';
    // 2
    expected[4 + 4] = '0';
    expected[5 + 4] = '0';
    expected[6 + 4] = '1';
    expected[7 + 4] = '0';
    // 3
    expected[8 + 4] = '0';
    expected[9 + 4] = '0';
    expected[10 + 4] = '1';
    expected[11 + 4] = '1';
    // 4
    expected[12 + 4] = '0';
    expected[13 + 4] = '1';
    expected[14 + 4] = '0';
    expected[15 + 4] = '0';
    // 5
    expected[16 + 4] = '0';
    expected[17 + 4] = '1';
    expected[18 + 4] = '0';
    expected[19 + 4] = '1';
    // 6
    expected[20 + 4] = '0';
    expected[21 + 4] = '1';
    expected[22 + 4] = '1';
    expected[23 + 4] = '0';
    // 7
    expected[24 + 4] = '0';
    expected[25 + 4] = '1';
    expected[26 + 4] = '1';
    expected[27 + 4] = '1';
    // 8
    expected[28 + 4] = '1';
    expected[29 + 4] = '0';
    expected[30 + 4] = '0';
    expected[31 + 4] = '0';
    // '\0'
    expected[32 + 4] = 0;

    ithU32tob(&buf[4], 0x12345678U);

    dump_hex(buf, sizeof(buf));

    EXPECT_EQ(memcmp(buf, expected, sizeof(buf)), 0);
}
