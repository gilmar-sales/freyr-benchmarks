#include <benchmark/benchmark.h>
#include <glm/glm.hpp>

struct DynamicObject
{
    glm::vec3 position{};
    glm::vec3 velocity{};
    glm::vec3 acceleration = { 1.0f, 3.0f, 2.0f };
    glm::vec3 gravity      = { 0.0f, -9.81f, 0.0f };
};

constexpr auto   DELTA_TIME    = 1.0f / 60.0f;
constexpr size_t L1_CACHE_SIZE = 32 * 1024; // 32 KB típico
constexpr size_t BLOCK_SIZE    = L1_CACHE_SIZE / sizeof(DynamicObject) - 2;

static void Sistema_Gravidade(DynamicObject& gameObject)
{
    gameObject.acceleration += gameObject.gravity * DELTA_TIME;
}

static void Sistema_Aceleracao(DynamicObject& gameObject)
{
    gameObject.velocity += gameObject.acceleration * DELTA_TIME;
}

static void Sistema_Velocidade(DynamicObject& gameObject)
{
    gameObject.position += gameObject.velocity * DELTA_TIME;
}

static void ConjuntoTrabalho_PorSistema(benchmark::State& state)
{
    auto gameObjects = std::vector<DynamicObject>(state.range());

    for (auto _ : state)
    {
        state.PauseTiming();
        std::fill(gameObjects.begin(), gameObjects.end(), DynamicObject {});
        state.ResumeTiming();

        for (auto& gameObject : gameObjects)
        {
            Sistema_Gravidade(gameObject);
        }

        for (auto& gameObject : gameObjects)
        {
            Sistema_Aceleracao(gameObject);
        }

        for (auto& gameObject : gameObjects)
        {
            Sistema_Velocidade(gameObject);
        }
    }
}

static void ConjuntoTrabalho_PorObjeto(benchmark::State& state)
{
    auto gameObjects = std::vector<DynamicObject>(state.range());

    for (auto _ : state)
    {
        state.PauseTiming();
        std::fill(gameObjects.begin(), gameObjects.end(), DynamicObject {});
        state.ResumeTiming();

        for (auto& gameObject : gameObjects)
        {
            Sistema_Gravidade(gameObject);
            Sistema_Aceleracao(gameObject);
            Sistema_Velocidade(gameObject);
        }
    }
}

static void ConjuntoTrabalho_PorBlocos(benchmark::State& state)
{
    auto         gameObjects = std::vector<DynamicObject>(state.range());
    const size_t total       = gameObjects.size();

    for (auto _ : state)
    {
        for (size_t block_start = 0; block_start < total; block_start += BLOCK_SIZE)
        {
            const size_t block_end = std::min(block_start + BLOCK_SIZE, total);

            for (size_t i = block_start; i < block_end; ++i)
            {
                Sistema_Gravidade(gameObjects[i]);
            }

            for (size_t i = block_start; i < block_end; ++i)
            {
                Sistema_Aceleracao(gameObjects[i]);
            }

            for (size_t i = block_start; i < block_end; ++i)
            {
                Sistema_Velocidade(gameObjects[i]);
            }
        }
    }
}

BENCHMARK(ConjuntoTrabalho_PorSistema)
    ->Arg(1000'000)
    ->Repetitions(100)
    ->Iterations(10)
    ->ReportAggregatesOnly(true);

BENCHMARK(ConjuntoTrabalho_PorObjeto)
    ->Arg(1000'000)
    ->Repetitions(100)
    ->Iterations(10)
    ->ReportAggregatesOnly(true);

BENCHMARK(ConjuntoTrabalho_PorBlocos)
    ->Arg(1000'000)
    ->Repetitions(100)
    ->Iterations(10)
    ->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
