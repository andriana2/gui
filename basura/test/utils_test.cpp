#include <gtest/gtest.h>
#include "../lib/utils.h"

TEST(obtein_ip, EXPECT_EQ)
{
    EXPECT_EQ("192.168.0.2", obtenerIP()); // interiores
    EXPECT_EQ("192.168.1.137", obtenerIP()); // exteriores
}
