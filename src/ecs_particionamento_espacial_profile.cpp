#include <random>

#include <Freyr/Freyr.hpp>
#include <benchmark/benchmark.h>
#include <entt/entt.hpp>

#include "Components/SphereColliderComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Containers/Octree.hpp"
#include "Skirnir/Common.hpp"

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

struct EmptyTag
{
};

struct Position : fr::Component
{
    float x;
    float y;
    float z;
};

auto arena = ArenaAllocator();

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
static void ECS_Particionamento_Freyr_Iniciar(size_t entity_count, size_t thread_count)
{
    app = skr::ApplicationBuilder()
              .WithExtension<fr::FreyrExtension>([&](fr::FreyrExtension& freyr) {
                  freyr.WithOptions([&](fr::FreyrOptionsBuilder& freyrOptions) {
                           freyrOptions.WithMaxEntities(entity_count).WithThreadCount(thread_count).WithArchetypeChunkCapacity(4 * 1024);
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
        .WithEntities(entity_count)
        .Build();

    scene->ExecuteTasks();
}

static void ECS_Particionamento_Freyr_Assinc()
{
    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    arena.reset();
    auto octree = arena.construct<Octree>(glm::vec3(0.0f), 100'000.0f, &arena);

    scene->CreateQuery()->EachAsync<TransformComponent, SphereColliderComponent>([octree](const fr::Entity entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
        const auto particle = Particle {
            .entity         = entity,
            .transform      = transform,
            .sphereCollider = sphereCollider
        };

        octree->Insert(particle);
    });

    scene->ExecuteTasks();

    scene->CreateQuery()->EachAsync<TransformComponent, SphereColliderComponent>([octree](const fr::Entity entity, TransformComponent& transform, SphereColliderComponent& sphereCollider) {
        auto particle = Particle {
            .entity         = entity,
            .transform      = transform,
            .sphereCollider = sphereCollider
        };

        auto collisions = std::vector<Particle>();
        octree->Query(particle, collisions);
    });

    scene->ExecuteTasks();

    benchmark::DoNotOptimize(octree);
}

entt::basic_registry<size_t>* registry;

static void ECS_Particionamento_Entt_Paralelo_Iniciar(size_t entity_count)
{
    if (registry != nullptr)
        delete registry;

    registry = new entt::basic_registry<size_t>;

    for (auto i = 0u; i < entity_count; ++i)
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

std::atomic<long> trackSequence  = 200;
const auto        TrackPrincipal = perfetto::ProcessTrack::Current();

static void ECS_Particionamento_Entt_Paralelo(size_t entity_count, size_t num_threads)
{
    auto view = registry->view<TransformComponent, SphereColliderComponent>();

    const auto chunk_size = entity_count / num_threads;
    arena.reset();
    auto octree = arena.construct<Octree>(glm::vec3(0.0f), 100'000.0f, &arena);

    std::vector<std::thread> threads;

    threads.reserve(num_threads);

    for (size_t i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&, i]() {
            const auto  start = i * chunk_size;
            const auto  end   = (i == num_threads - 1) ? entity_count : (i + 1) * chunk_size;
            const auto& track = perfetto::Track::ThreadScoped((void*) trackSequence.fetch_add(1), TrackPrincipal);
            FREYR_TRACE_BEGIN("USER", "ECS_Iteracao_EnTT_Paralelo", track, "EntityCount", end - start);

            auto it    = view.begin();
            auto endIt = view.end();
            std::advance(it, start);

            for (size_t j = start; j < end && it != endIt; ++j, ++it)
            {
                auto [transform, sphereCollider] = view.get(*it);
                const auto particle              = Particle {
                                 .entity         = static_cast<size_t>(*it),
                                 .transform      = transform,
                                 .sphereCollider = sphereCollider
                };

                octree->Insert(particle);
            }
            FREYR_TRACE_END("USER", track);
        });
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }

    threads.clear();
    trackSequence = 200;
    for (size_t i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&, i]() {
            const auto  start = i * chunk_size;
            const auto  end   = (i == num_threads - 1) ? entity_count : (i + 1) * chunk_size;
            const auto& track = perfetto::Track::ThreadScoped((void*) trackSequence.fetch_add(1), TrackPrincipal);
            FREYR_TRACE_BEGIN("USER", "ECS_Iteracao_EnTT_Paralelo", track, "EntityCount", end - start);

            auto it    = view.begin();
            auto endIt = view.end();
            std::advance(it, start);

            for (size_t j = start; j < end && it != endIt; ++j, ++it)
            {
                auto [transform, sphereCollider] = view.get(*it);
                auto particle                    = Particle {
                                       .entity         = static_cast<size_t>(*it),
                                       .transform      = transform,
                                       .sphereCollider = sphereCollider
                };

                auto collisions = std::vector<Particle>();
                octree->Query(particle, collisions);
            }
            FREYR_TRACE_END("USER", track);
        });
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }

    benchmark::DoNotOptimize(octree);
}

int main(int argc, char const* argv[])
{
    size_t entity_count = 100'000;
    size_t thread_count = 8;

    // ECS_Particionamento_Entt_Paralelo_Iniciar(entity_count);
    ECS_Particionamento_Freyr_Iniciar(entity_count, thread_count);

    app->GetRootServiceProvider()->GetService<fr::Scene>()->BeginProfiling();
    app->GetRootServiceProvider()->GetService<fr::Scene>()->Update(0.0f);

    // ECS_Particionamento_Entt_Paralelo(entity_count, thread_count);
    ECS_Particionamento_Freyr_Assinc();
    app->GetRootServiceProvider()->GetService<fr::Scene>()->EndProfiling();

    return 0;
}
