#include <Freyr/Freyr.hpp>
#include <benchmark/benchmark.h>
#include <entt/entt.hpp>
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

Ref<MyApp> app;
void       ECS_Iteracao_Freyr_Iniciar(const benchmark::State& state)
{
    app = skr::ApplicationBuilder()
              .WithExtension<fr::FreyrExtension>([&](fr::FreyrExtension& freyr) {
                  freyr.WithOptions([&](fr::FreyrOptionsBuilder& freyrOptions) {
                           freyrOptions.WithMaxEntities(state.range()).WithArchetypeChunkCapacity(state.range(1));
                       })
                      .WithComponent<Position>()
                      .WithComponent<Velocity>()
                      .WithComponent<Acceleration>();
              })
              .Build<MyApp>();

    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    scene->CreateArchetypeBuilder()
        .WithComponent(Position {})
        .WithComponent(Velocity {})
        .WithComponent(Acceleration { .x = 1.0f, .y = 1.0f, .z = 0.0f })
        .WithEntities(state.range())
        .Build();
}

static void ECS_Iteracao_Freyr(benchmark::State& state)
{
    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    for (auto _ : state)
    {
        scene->CreateQuery()->Each<Position, Velocity, Acceleration>(MovementSystem);
    }
}

entt::basic_registry<size_t>* registry;

static void ECS_Iteracao_Entt_Iniciar(const benchmark::State& state)
{
    if (registry != nullptr)
        delete registry;

    registry = new entt::basic_registry<size_t>;

    for (auto i = 0u; i < state.range(); ++i)
    {
        const auto entity = registry->create();
        registry->emplace<Position>(entity, Position {});
        registry->emplace<Velocity>(entity, Velocity {});
        registry->emplace<Acceleration>(entity, Acceleration { .x = 1.0f, .y = 1.0f, .z = 0.0f });
    }
}

static void ECS_Iteracao_Entt(benchmark::State& state)
{
    const auto group = registry->group<Position, Velocity, Acceleration>();

    for (auto _ : state)
    {
        group.each(MovementSystem);
    }
}

flecs::world* flecsWorld = nullptr;

static void ECS_Iteracao_Flecs_Iniciar(const benchmark::State& state)
{
    flecsWorld = new flecs::world;
    flecsWorld->set_threads(1);
    flecsWorld->set_task_threads(1);

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

static void ECS_Iteracao_Flecs(benchmark::State& state)
{
    for (auto _ : state)
    {
        flecsWorld->each(MovementSystem);
    }
}

BENCHMARK(ECS_Iteracao_Freyr)
    ->Setup(ECS_Iteracao_Freyr_Iniciar)
    ->Iterations(1000)
    ->ArgsProduct({
        benchmark::CreateDenseRange(4'000'000, 10'000'000, 1'000'000),
        benchmark::CreateDenseRange(512, 4096, 3584),
    })
    ->ReportAggregatesOnly(true);

BENCHMARK(ECS_Iteracao_Entt)
    ->Setup(ECS_Iteracao_Entt_Iniciar)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000)
    ->ReportAggregatesOnly(true);

BENCHMARK(ECS_Iteracao_Flecs)
    ->Setup(ECS_Iteracao_Flecs_Iniciar)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000)
    ->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
