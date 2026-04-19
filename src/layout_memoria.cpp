#include <benchmark/benchmark.h>

struct PositionAoS
{
    float x;
    float y;
    float z;
};

struct PositionSoA
{
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> z;
};

static void ArranjoDeEstruturas_XYZ(benchmark::State& state)
{
    auto positions = std::vector<PositionAoS>(state.range());

    for (auto _ : state)
    {
        for (auto& position : positions)
        {
            position.x += 1.0f;
            position.y += 1.0f;
            position.z += 1.0f;
        }
    }
}

static void EstruturaDeArranjos_XYZ(benchmark::State& state)
{
    auto positions = PositionSoA {
        .x = std::vector<float>(state.range()),
        .y = std::vector<float>(state.range()),
        .z = std::vector<float>(state.range())
    };

    for (auto _ : state)
    {
        for (auto i = 0; i < state.range(); i++)
        {
            positions.x[i] += 1.0f;
            positions.y[i] += 1.0f;
            positions.z[i] += 1.0f;
        }
    }
}


static void ArranjoDeEstruturas_UmaCoordenadaPorVez(benchmark::State& state)
{
    auto positions = std::vector<PositionAoS>(state.range());

    for (auto _ : state)
    {
        for (auto& position : positions)
        {
            position.x += 1.0f;
        }

        for (auto& position : positions)
        {
            position.y += 1.0f;
        }

        for (auto& position : positions)
        {
            position.z += 1.0f;
        }
    }
}

static void EstruturaDeArranjos_UmaCoordenadaPorVez(benchmark::State& state)
{
    auto positions = PositionSoA {
        .x = std::vector<float>(state.range()),
        .y = std::vector<float>(state.range()),
        .z = std::vector<float>(state.range())
    };

    for (auto _ : state)
    {
        for (auto i = 0; i < state.range(); i++)
        {
            positions.x[i] += 1.0f;
        }

        for (auto i = 0; i < state.range(); i++)
        {
            positions.y[i] += 1.0f;
        }

        for (auto i = 0; i < state.range(); i++)
        {
            positions.z[i] += 1.0f;
        }
    }
}

BENCHMARK(ArranjoDeEstruturas_XYZ)->Iterations(1'000)->RangeMultiplier(2)->Range(1, 10'000'000);
BENCHMARK(EstruturaDeArranjos_XYZ)->Iterations(1'000)->RangeMultiplier(2)->Range(1, 10'000'000);

BENCHMARK(EstruturaDeArranjos_UmaCoordenadaPorVez)->Iterations(1'000)->RangeMultiplier(2)->Range(1, 10'000'000);
BENCHMARK(ArranjoDeEstruturas_UmaCoordenadaPorVez)->Iterations(1'000)->RangeMultiplier(2)->Range(1, 10'000'000);

BENCHMARK_MAIN();
