#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <vector>

const std::size_t COLS = 200;
const std::size_t A_ROWS = 200;
const std::size_t B_COLS = 200;

#if defined(__GNUC__) || defined(__clang__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

// Compile-time column stride of 1.
inline std::size_t idxc(std::size_t i, std::size_t j,
                        std::size_t row_stride)
{
    constexpr std::size_t COL_STRIDE = 1;
    return i * row_stride + j * COL_STRIDE;
}

// Runtime column stride.
inline std::size_t idxr(std::size_t i, std::size_t j,
                        std::size_t row_stride,
                        std::size_t col_stride)
{
    return i * row_stride + j * col_stride;
}

NOINLINE void comptime(std::vector<float> &out,
                       const std::vector<float> &a,
                       const std::vector<float> &b)
{
    for (std::size_t i = 0; i < A_ROWS; ++i)
    {
        for (std::size_t k = 0; k < COLS; ++k)
        {
            const float a_ik = a[idxc(i, k, COLS)];
            for (std::size_t j = 0; j < B_COLS; ++j)
            {
                out[idxc(i, j, B_COLS)] +=
                    a_ik * b[idxc(k, j, B_COLS)];
            }
        }
    }
}

NOINLINE void runtime(std::vector<float> &out,
                      const std::vector<float> &a,
                      const std::vector<float> &b,
                      std::size_t a_col_stride,
                      std::size_t b_col_stride,
                      std::size_t out_col_stride)
{
    for (std::size_t i = 0; i < A_ROWS; ++i)
    {
        for (std::size_t k = 0; k < COLS; ++k)
        {
            const float a_ik =
                a[idxr(i, k, COLS, a_col_stride)];
            for (std::size_t j = 0; j < B_COLS; ++j)
            {
                out[idxr(i, j, B_COLS, out_col_stride)] +=
                    a_ik * b[idxr(k, j, B_COLS, b_col_stride)];
            }
        }
    }
}

int main(int argc, char *argv[])
{
    // Read it from argv so that it is a runtime value
    std::size_t stride = std::stoull(argv[1]);
    // Same idea as black_box(1): obtain the stride through a volatile
    // variable so the compiler must load its value at runtime.
    // volatile std::size_t stride = 1;
    std::vector<float> a(A_ROWS * COLS);
    std::vector<float> b(COLS * B_COLS);

    for (std::size_t i = 0; i < a.size(); ++i)
        a[i] = static_cast<float>(i) * 0.1f;
    for (std::size_t i = 0; i < b.size(); ++i)
        b[i] = static_cast<float>(i) * 0.2f;

    std::vector<float> outc(A_ROWS * B_COLS, 0.0f);
    std::vector<float> outr(A_ROWS * B_COLS, 0.0f);

    // Warm-up runs
    for (int i = 0; i < 10; ++i)
    {
        comptime(outc, a, b);
    }

    // We could do more samples and switch the order
    // but I am keeping it simple
    // Letting runtime run last
    const auto tic_c = std::chrono::steady_clock::now();
    comptime(outc, a, b);
    const auto elapsed_c =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - tic_c)
            .count() /
        1000.0f;

    for (int i = 0; i < 10; ++i)
    {
        runtime(outr, a, b, stride, stride, stride);
    }

    const auto tic_r = std::chrono::steady_clock::now();
    runtime(outr, a, b, stride, stride, stride);
    const auto elapsed_r =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - tic_r)
            .count() /
        1000.0f;

    for (std::size_t i = 0; i < outc.size(); ++i)
    {
        assert(outc[i] == outr[i]);
    }

    std::cout << "Stride=1 at compile time: "
              << elapsed_c << " ms\n";
    std::cout << "Stride=" << stride << " at runtime: "
              << elapsed_r << " ms\n";
    const float ratio = elapsed_r / elapsed_c;
    std::cout << std::format("Ratio: {:.2}x times slower\n", ratio);

    return 0;
}
