#define CATCH_CONFIG_MAIN
#include "catch.hpp"

template <int c, int bit> struct mask_recurse {
  static int foo(int n) {
    return (((~c ^ n) >> bit) & 1) && mask_recurse<c, bit - 1>::foo(n);
  }
};

template <int c> struct mask_recurse<c, -1> {
  static int foo(int n) { return 1; }
};

template <int c=5> int mask(int n) { return mask_recurse<c, 31>::foo(n) - 1; }

TEST_CASE("foo") {
  volatile int x = 5;
  REQUIRE(mask<5>(x) == 0);
  REQUIRE(mask<5>(6) == -1);
}
