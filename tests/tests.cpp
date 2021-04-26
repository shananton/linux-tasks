#include <gtest/gtest.h>

#include "transform.hpp"

#include <cstddef>
#include <numeric>
#include <vector>
#include <ranges>
#include <iostream>

uint64_t quadratic_spin(uint64_t x) {
  for (uint64_t i = 0; i < x * x; ++i) {
    asm volatile("");
  }
  return x;
}

uint64_t linear_spin(uint64_t x) {
  for (uint64_t i = 0; i < x; ++i) {
    asm volatile("");
  }
  return x;
}

TEST(Correctness, TransformStatic) {
  constexpr size_t SIZE = 10'000;
  std::vector<uint64_t> xs(SIZE);
  std::iota(xs.begin(), xs.end(), 0);
  auto correct = xs;

  for (size_t nprocesses : {1, 2, 3, 4, 8, 16, 100}) {
    auto transformed = transform::transform_static(xs, linear_spin, nprocesses);
    ASSERT_TRUE(std::ranges::equal(correct, transformed));
  }
}

TEST(Correctness, TransformDynamic) {
  constexpr size_t SIZE = 10'000;
  std::vector<uint64_t> xs(SIZE);
  std::iota(xs.begin(), xs.end(), 0);
  auto correct = xs;

  for (size_t nprocesses : {1, 2, 3, 4, 8, 16, 100}) {
    for (size_t chunk_size : {1, 2, 4, 8, 16, 32, 64, 128, 1024, 8192}) {
      auto transformed = transform::transform_dynamic(xs, linear_spin, nprocesses, chunk_size);
      ASSERT_TRUE(std::ranges::equal(correct, transformed));
    }
  }
}

class Linear100kSpeed : public ::testing::Test {
  protected:
    void SetUp() override {
      xs.resize(SIZE);
      std::iota(xs.begin(), xs.end(), 0);
    }

    static constexpr size_t SIZE = 100'000;
    std::vector<int> xs;
};

class Quadratic5kSpeed : public ::testing::Test {
  protected:
    void SetUp() override {
      xs.resize(SIZE);
      std::iota(xs.begin(), xs.end(), 0);
    }

    static constexpr size_t SIZE = 5'000;
    std::vector<int> xs;
};

TEST_F(Linear100kSpeed, TransformStaticJ1) {
  ASSERT_EQ(0, *transform::transform_static(xs, linear_spin, 1).begin());
}
TEST_F(Linear100kSpeed, TransformStaticJ2) {
  ASSERT_EQ(0, *transform::transform_static(xs, linear_spin, 2).begin());
}
TEST_F(Linear100kSpeed, TransformStaticJ4) {
  ASSERT_EQ(0, *transform::transform_static(xs, linear_spin, 4).begin());
}
TEST_F(Linear100kSpeed, TransformStaticJ8) {
  ASSERT_EQ(0, *transform::transform_static(xs, linear_spin, 8).begin());
}
TEST_F(Linear100kSpeed, TransformStaticJ16) {
  ASSERT_EQ(0, *transform::transform_static(xs, linear_spin, 16).begin());
}
TEST_F(Linear100kSpeed, TransformStaticJ512) {
  ASSERT_EQ(0, *transform::transform_static(xs, linear_spin, 512).begin());
}

TEST_F(Linear100kSpeed, TransformDynamicJ1) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, linear_spin, 1, 10).begin());
}
TEST_F(Linear100kSpeed, TransformDynamicJ2) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, linear_spin, 2, 10).begin());
}
TEST_F(Linear100kSpeed, TransformDynamicJ4) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, linear_spin, 4, 10).begin());
}
TEST_F(Linear100kSpeed, TransformDynamicJ8) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, linear_spin, 8, 10).begin());
}
TEST_F(Linear100kSpeed, TransformDynamicJ16) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, linear_spin, 16, 10).begin());
}
TEST_F(Linear100kSpeed, TransformDynamicJ512) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, linear_spin, 512, 10).begin());
}


TEST_F(Quadratic5kSpeed, TransformStaticJ1) {
  ASSERT_EQ(0, *transform::transform_static(xs, quadratic_spin, 1).begin());
}
TEST_F(Quadratic5kSpeed, TransformStaticJ2) {
  ASSERT_EQ(0, *transform::transform_static(xs, quadratic_spin, 2).begin());
}
TEST_F(Quadratic5kSpeed, TransformStaticJ4) {
  ASSERT_EQ(0, *transform::transform_static(xs, quadratic_spin, 4).begin());
}
TEST_F(Quadratic5kSpeed, TransformStaticJ8) {
  ASSERT_EQ(0, *transform::transform_static(xs, quadratic_spin, 8).begin());
}
TEST_F(Quadratic5kSpeed, TransformStaticJ16) {
  ASSERT_EQ(0, *transform::transform_static(xs, quadratic_spin, 16).begin());
}
TEST_F(Quadratic5kSpeed, TransformStaticJ512) {
  ASSERT_EQ(0, *transform::transform_static(xs, quadratic_spin, 512).begin());
}

TEST_F(Quadratic5kSpeed, TransformDynamicJ1) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, quadratic_spin, 1, 10).begin());
}
TEST_F(Quadratic5kSpeed, TransformDynamicJ2) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, quadratic_spin, 2, 10).begin());
}
TEST_F(Quadratic5kSpeed, TransformDynamicJ4) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, quadratic_spin, 4, 10).begin());
}
TEST_F(Quadratic5kSpeed, TransformDynamicJ8) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, quadratic_spin, 8, 10).begin());
}
TEST_F(Quadratic5kSpeed, TransformDynamicJ16) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, quadratic_spin, 16, 10).begin());
}
TEST_F(Quadratic5kSpeed, TransformDynamicJ512) {
  ASSERT_EQ(0, *transform::transform_dynamic(xs, quadratic_spin, 512, 10).begin());
}
