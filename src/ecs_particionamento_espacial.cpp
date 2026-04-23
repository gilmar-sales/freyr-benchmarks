#include <random>
#include <thread>

#include <benchmark/benchmark.h>

#include <Freyr/Freyr.hpp>

#include <entt/entt.hpp>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/tbb.h>

#include <flecs.h>

#include "Components/SphereColliderComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Containers/Octree.hpp"

template <typename T>
static T randomNumber(const T min, const T max)
{
    if constexpr (std::is_floating_point_v<T>)
    {
        std::random_device                r;
        std::default_random_engine        e1(r());
        std::uniform_real_distribution<T> uniform_dist(min, max);

        return uniform_dist(e1);
    }
    else
    {
        std::random_device               r;
        std::default_random_engine       e1(r());
        std::uniform_int_distribution<T> uniform_dist(min, max);

        return uniform_dist(e1);
    }
}

static glm::vec3 randomPosition(float min, float max)
{
    return { randomNumber(min, max), randomNumber(min, max), randomNumber(min, max) };
}

struct Position : fr::Component
{
    float x;
    float y;
    float z;
};

auto    arena = ArenaAllocator();
Octree* octree;

static void BuildOctree(size_t entity, TransformComponent& transform, SphereColliderComponent& sphereCollider)
{
    const auto particle = Particle {
        .entity         = entity,
        .transform      = transform,
        .sphereCollider = sphereCollider
    };
    octree->Insert(particle);
}

static void QueryOctree(std::vector<Particle>& found, size_t entity, TransformComponent& transform, SphereColliderComponent& sphereCollider)
{
    auto particle = Particle {
        .entity         = entity,
        .transform      = transform,
        .sphereCollider = sphereCollider
    };
    octree->Query(particle, found);
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

Ref<MyApp>  app;
static void ECS_Particionamento_Freyr_Iniciar(const benchmark::State& state)
{
    app = skr::ApplicationBuilder()
              .WithExtension<fr::FreyrExtension>([&](fr::FreyrExtension& freyr) {
                  freyr.WithOptions([&](fr::FreyrOptionsBuilder& freyrOptions) {
                           freyrOptions.WithMaxEntities(state.range()).WithThreadCount(std::thread::hardware_concurrency() - 1);
                       })
                      .WithComponent<TransformComponent>();
              })
              .Build<MyApp>();

    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    scene->CreateArchetypeBuilder()
        .WithComponent(TransformComponent {})
        .WithComponent(SphereColliderComponent {})
        .ForEach<TransformComponent, SphereColliderComponent>([](auto entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
            transform.position = randomPosition(-100'000.0f, 100'000.0f);
            transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            transform.scale    = glm::vec3(1.0f);

            sphereCollider.radius = randomNumber(5.0f, 15.0f);
            sphereCollider.offset = glm::vec3(0.0f);
        })
        .WithEntities(state.range())
        .Build();

    scene->ExecuteTasks();
}

static void ECS_Particionamento_Freyr(benchmark::State& state)
{
    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    for (auto _ : state)
    {
        arena.reset();
        octree = arena.construct<Octree>(glm::vec3(0.0f), 100'000.0f, &arena);

        scene->CreateQuery()->Each<TransformComponent, SphereColliderComponent>([](const fr::Entity entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
            BuildOctree(entity, transform, sphereCollider);
        });

        scene->CreateQuery()->Each<TransformComponent, SphereColliderComponent>([](const fr::Entity entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
            auto collisions = std::vector<Particle>();
            QueryOctree(collisions, entity, transform, sphereCollider);
            benchmark::DoNotOptimize(collisions);
        });

        benchmark::DoNotOptimize(octree);
    }
}

static void ECS_Particionamento_Freyr_Assinc(benchmark::State& state)
{
    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    for (auto _ : state)
    {
        arena.reset();
        octree = arena.construct<Octree>(glm::vec3(0.0f), 100'000.0f, &arena);

        scene->CreateQuery()->EachAsync<TransformComponent, SphereColliderComponent>([](const fr::Entity entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
            BuildOctree(entity, transform, sphereCollider);
        });

        scene->ExecuteTasks();

        scene->CreateQuery()->EachAsync<TransformComponent, SphereColliderComponent>([](const fr::Entity entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
            auto collisions = std::vector<Particle>();
            QueryOctree(collisions, entity, transform, sphereCollider);
            benchmark::DoNotOptimize(collisions);
        });

        scene->ExecuteTasks();

        benchmark::DoNotOptimize(octree);
    }
}

entt::basic_registry<size_t>* registry;

static void ECS_Particionamento_Entt_Paralelo_Iniciar(const benchmark::State& state)
{
    if (registry != nullptr)
        delete registry;

    registry = new entt::basic_registry<size_t>;

    for (auto i = 0u; i < state.range(0); ++i)
    {
        const auto entity = registry->create();
        registry->emplace<TransformComponent>(entity, TransformComponent {
                                                          .position = randomPosition(-100'000.0f, 100'000.0f),
                                                          .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                                                          .scale    = glm::vec3(1.0f),
                                                      });
        registry->emplace<SphereColliderComponent>(entity, SphereColliderComponent {
                                                               .radius = randomNumber(5.0f, 15.0f),
                                                               .offset = glm::vec3(0.0f),
                                                           });
    }
}

static void ECS_Particionamento_Entt(benchmark::State& state)
{
    auto group = registry->group<TransformComponent, SphereColliderComponent>();

    for (auto _ : state)
    {
        arena.reset();
        octree = arena.construct<Octree>(glm::vec3(0.0f), 100'000.0f, &arena);

        group.each([](const auto entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
            BuildOctree(entity, transform, sphereCollider);
        });

        group.each([](const auto entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
            auto collisions = std::vector<Particle>();
            QueryOctree(collisions, entity, transform, sphereCollider);
            benchmark::DoNotOptimize(collisions);
        });

        benchmark::DoNotOptimize(octree);
    }
}

static void ECS_Particionamento_Entt_TBB(benchmark::State& state)
{
    auto group = registry->group<TransformComponent, SphereColliderComponent>();

    auto count = state.range(0);
    for (auto _ : state)
    {
        arena.reset();
        octree = arena.construct<Octree>(glm::vec3(0.0f), 100'000.0f, &arena);

        tbb::parallel_for(tbb::blocked_range<size_t>(0, count), [&group](const tbb::blocked_range<size_t>& range) {
            for (auto i = range.begin(); i != range.end(); ++i)
            {
                auto entity                      = group[i];
                auto [transform, sphereCollider] = group.get(entity);
                BuildOctree(entity, transform, sphereCollider);
            }
        });

        tbb::parallel_for(tbb::blocked_range<size_t>(0, count), [&group](const tbb::blocked_range<size_t>& range) {
            for (auto i = range.begin(); i != range.end(); ++i)
            {
                auto entity                      = group[i];
                auto [transform, sphereCollider] = group.get(entity);
                auto particle                    = Particle {
                    .entity         = static_cast<size_t>(entity),
                    .transform      = transform,
                    .sphereCollider = sphereCollider
                };
                auto collisions = std::vector<Particle>();
                QueryOctree(collisions, entity, transform, sphereCollider);
                benchmark::DoNotOptimize(collisions);
            }
        });

        benchmark::DoNotOptimize(octree);
    }
}

flecs::world* flecsWorld = nullptr;
flecs::system buildOctreeSystem;
flecs::system queryOctreeSystem;

static void ECS_Iteracao_Flecs_Iniciar(const benchmark::State& state)
{
    flecsWorld = new flecs::world;
    flecsWorld->set_threads(1);
    buildOctreeSystem =
        flecsWorld->system<TransformComponent, SphereColliderComponent>()
            .each([](size_t entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
                BuildOctree(entity, transform, sphereCollider);
            });

    queryOctreeSystem =
        flecsWorld->system<TransformComponent, SphereColliderComponent>()
            .each([](size_t entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
                auto collisions = std::vector<Particle>();
                QueryOctree(collisions, entity, transform, sphereCollider);
                benchmark::DoNotOptimize(collisions);
            });

    flecsWorld->component<TransformComponent>();
    flecsWorld->component<SphereColliderComponent>();

    for (auto i = 0u; i < state.range(); ++i)
    {
        flecsWorld->entity()
            .set<TransformComponent>({
                .position = randomPosition(-100'000.0f, 100'000.0f),
                .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                .scale    = glm::vec3(1.0f),
            })
            .set<SphereColliderComponent>({
                .radius = randomNumber(5.0f, 15.0f),
                .offset = glm::vec3(0.0f),
            });
    }
}
static void ECS_Iteracao_Flecs_Iniciar_multi(const benchmark::State& state)
{
    flecsWorld = new flecs::world;
    flecsWorld->set_threads(std::thread::hardware_concurrency() - 1);

    buildOctreeSystem =
        flecsWorld->system<TransformComponent, SphereColliderComponent>()
            .multi_threaded()
            .each([](size_t entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
                BuildOctree(entity, transform, sphereCollider);
            });

    queryOctreeSystem =
        flecsWorld->system<TransformComponent, SphereColliderComponent>()
            .multi_threaded()
            .each([](size_t entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
                BuildOctree(entity, transform, sphereCollider);
            });

    flecsWorld->component<TransformComponent>();
    flecsWorld->component<SphereColliderComponent>();

    for (auto i = 0u; i < state.range(); ++i)
    {
        flecsWorld->entity()
            .set<TransformComponent>({
                .position = randomPosition(-100'000.0f, 100'000.0f),
                .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                .scale    = glm::vec3(1.0f),
            })
            .set<SphereColliderComponent>({
                .radius = randomNumber(5.0f, 15.0f),
                .offset = glm::vec3(0.0f),
            });
    }
}

static void ECS_Iteracao_Flecs(benchmark::State& state)
{
    buildOctreeSystem =
        flecsWorld->system<TransformComponent, SphereColliderComponent>()
            .each([](size_t entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
                BuildOctree(entity, transform, sphereCollider);
            });

    queryOctreeSystem =
        flecsWorld->system<TransformComponent, SphereColliderComponent>()
            .each([](size_t entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
                auto collisions = std::vector<Particle>();
                QueryOctree(collisions, entity, transform, sphereCollider);
                benchmark::DoNotOptimize(collisions);
            });

    for (auto _ : state)
    {
        arena.reset();
        octree = arena.construct<Octree>(glm::vec3(0.0f), 100'000.0f, &arena);

        buildOctreeSystem.run();
        queryOctreeSystem.run();
    }
}

static void ECS_Iteracao_Flecs_Multi(benchmark::State& state)
{
    for (auto _ : state)
    {
        arena.reset();
        octree = arena.construct<Octree>(glm::vec3(0.0f), 100'000.0f, &arena);

        flecsWorld->progress();
    }
}

BENCHMARK(ECS_Particionamento_Entt)
    ->Setup(ECS_Particionamento_Entt_Paralelo_Iniciar)
    ->DenseRange(1'000, 100'000, 9'900);

BENCHMARK(ECS_Particionamento_Freyr)
    ->Setup(ECS_Particionamento_Freyr_Iniciar)
    ->DenseRange(1'000, 100'000, 9'900);

BENCHMARK(ECS_Iteracao_Flecs)
    ->Setup(ECS_Iteracao_Flecs_Iniciar)
    ->DenseRange(1'000, 100'000, 9'900);

BENCHMARK(ECS_Particionamento_Entt_TBB)
    ->Setup(ECS_Particionamento_Entt_Paralelo_Iniciar)
    ->DenseRange(1'000, 100'000, 9'900);

BENCHMARK(ECS_Particionamento_Freyr_Assinc)
    ->Setup(ECS_Particionamento_Freyr_Iniciar)
    ->DenseRange(1'000, 100'000, 9'900);

BENCHMARK(ECS_Iteracao_Flecs_Multi)
    ->Setup(ECS_Iteracao_Flecs_Iniciar_multi)
    ->DenseRange(1'000, 100'000, 9'900);

BENCHMARK_MAIN();
