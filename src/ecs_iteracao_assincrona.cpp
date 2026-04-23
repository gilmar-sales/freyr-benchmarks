#include <thread>

#include <benchmark/benchmark.h>

#include <Freyr/Freyr.hpp>

#include <entt/entt.hpp>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/tbb.h>

#include <flecs.h>

struct Position : fr::Component
{
    float x;
    float y;
    float z;
};

struct Velocity : fr::Component
{
    float x;
    float y;
    float z;
};

struct Acceleration : fr::Component
{
    float x;
    float y;
    float z;
};

constexpr float DELTA_TIME = 1.0f / 60.0f;

static void MovementSystem(Position& position, Velocity& velocity, Acceleration& acceleration)
{
    velocity.x += acceleration.x * DELTA_TIME;
    velocity.y += acceleration.y * DELTA_TIME;
    velocity.y += acceleration.y * DELTA_TIME;

    position.x += velocity.x * DELTA_TIME;
    position.y += velocity.y * DELTA_TIME;
    position.z += velocity.z * DELTA_TIME;
}

class MyApp final : public skr::IApplication
{
  public:
    explicit MyApp(const Ref<skr::ServiceProvider>& rootServiceProvider) :
        IApplication(rootServiceProvider) {}

    void Run() override
    {
    }
};

constexpr auto BLOCK_SIZE = 4096;

Ref<MyApp>  app;
static void Freyr_Iniciar(const benchmark::State& state)
{
    app = skr::ApplicationBuilder()
              .WithExtension<fr::FreyrExtension>([&](fr::FreyrExtension& freyr) {
                  freyr.WithOptions([&](fr::FreyrOptionsBuilder& freyrOptions) {
                           freyrOptions.WithMaxEntities(state.range(0))
                               .WithThreadCount(std::thread::hardware_concurrency())
                               .WithArchetypeChunkCapacity(BLOCK_SIZE);
                       })
                      .WithComponent<Position>()
                      .WithComponent<Velocity>()
                      .WithComponent<Acceleration>();
              })
              .Build<MyApp>();

    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    scene->CreateArchetypeBuilder().WithComponent(Position {}).WithComponent(Velocity {}).WithComponent(Acceleration { .x = 1.0f, .y = 1.0f, .z = 0.0f }).WithEntities(state.range(0)).Build();
}

static void ECS_Iteracao_Freyr_RouboTrabalho(benchmark::State& state)
{
    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    for (auto _ : state)
    {
        scene->CreateQuery()->EachAsync<Position, Velocity, Acceleration>(MovementSystem);

        scene->ExecuteTasks();
    }
}

entt::basic_registry<size_t>* registry;

void EnTT_Iniciar(const benchmark::State& state)
{
    if (registry != nullptr)
        delete registry;

    registry          = new entt::basic_registry<size_t>;
    auto entity_count = state.range(0);
    auto num_threads  = state.range(1);

    for (auto i = 0u; i < entity_count; ++i)
    {
        const auto entity = registry->create();
        registry->emplace<Position>(entity, Position {});
        registry->emplace<Velocity>(entity, Velocity {});
        registry->emplace<Acceleration>(entity, Acceleration { .x = 1.0f, .y = 1.0f, .z = 0.0f });
    }
}

void ECS_Iteracao_EnTT_TBB(benchmark::State& state)
{
    auto group = registry->group<Position, Velocity, Acceleration>();
    auto count = state.range(0);

    for (auto _ : state)
    {
        tbb::parallel_for(tbb::blocked_range<size_t>(0, count, BLOCK_SIZE), [&group](const tbb::blocked_range<size_t>& range) {
            for (auto i = range.begin(); i != range.end(); ++i)
            {
                auto entity                             = group[i];
                auto [position, velocity, acceleration] = group.get(entity);

                MovementSystem(position, velocity, acceleration);
            }
        });
    }
}

flecs::world* flecsWorld = nullptr;
flecs::system flecsSystem;

static void ECS_Iteracao_Flecs_Iniciar(const benchmark::State& state)
{
    flecsWorld = new flecs::world;
    flecsWorld->set_threads(std::thread::hardware_concurrency() - 1);

    flecsSystem = flecsWorld->system<Position, Velocity, Acceleration>()
                      .multi_threaded()
                      .each(MovementSystem);

    flecsWorld->component<Position>();
    flecsWorld->component<Velocity>();
    flecsWorld->component<Acceleration>();

    for (auto i = 0u; i < state.range(); ++i)
    {
        flecsWorld->entity()
            .add<Position>()
            .add<Velocity>()
            .set<Acceleration>({ .x = 1.0f, .y = 1.0f, .z = 0.0f });
    }
}

static void ECS_Iteracao_Flecs_Multi(benchmark::State& state)
{
    for (auto _ : state)
    {
        flecsWorld->progress();
    }
}

BENCHMARK(ECS_Iteracao_EnTT_TBB)
    ->Setup(EnTT_Iniciar)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000);

BENCHMARK(ECS_Iteracao_Freyr_RouboTrabalho)
    ->Setup(Freyr_Iniciar)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000);

BENCHMARK(ECS_Iteracao_Flecs_Multi)
    ->Setup(ECS_Iteracao_Flecs_Iniciar)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000);

BENCHMARK_MAIN();
