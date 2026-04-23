#include <Freyr/Freyr.hpp>
#include <benchmark/benchmark.h>
#define ENTT_USE_ATOMIC
#include <entt/entt.hpp>

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
static void Freyr_Iniciar(size_t entity_count, size_t chunk_capacity, size_t thread_count)
{
    app = skr::ApplicationBuilder()
              .WithExtension<fr::FreyrExtension>([&](fr::FreyrExtension& freyr) {
                  freyr.WithOptions([&](fr::FreyrOptionsBuilder& freyrOptions) {
                           freyrOptions.WithMaxEntities(entity_count)
                               .WithArchetypeChunkCapacity(chunk_capacity)
                               .WithThreadCount(thread_count);
                       })
                      .WithComponent<Position>();
              })
              .Build<MyApp>();

    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    scene->CreateArchetypeBuilder().WithComponent(Position {}).WithComponent(Velocity { .x = 1.0f, .y = 2.0f, .z = 3.0f }).WithEntities(entity_count).Build();
}

static void ECS_Iteracao_Freyr_RouboTrabalho()
{
    auto scene = app->GetRootServiceProvider()->GetService<fr::Scene>();

    scene->CreateQuery()->EachAsync<Position, Velocity>([](const fr::Entity entity, Position& position, Velocity& velocity) {
        position.x += velocity.x;
        position.y += velocity.y;
        position.z += velocity.z;
    });

    scene->ExecuteTasks();
}

entt::basic_registry<size_t>* registry;

void EnTT_Iniciar(size_t entity_count)
{
    if (registry != nullptr)
        delete registry;

    registry = new entt::basic_registry<size_t>;

    for (auto i = 0u; i < entity_count; ++i)
    {
        const auto entity = registry->create();
        registry->emplace<Position>(entity, Position {});
        registry->emplace<Velocity>(entity, Velocity { .x = 1.0f, .y = 2.0f, .z = 3.0f });
    }
}

std::atomic<long> trackSequence  = 200;
const auto        TrackPrincipal = perfetto::ProcessTrack::Current();

void ECS_Iteracao_EnTT_Paralelo(size_t entity_count, size_t num_threads)
{
    const auto view       = registry->view<Position, Velocity>();
    const auto chunk_size = entity_count / num_threads;

    std::vector<std::thread> threads;

    for (size_t index = 0; index < num_threads; ++index)
    {
        threads.emplace_back([view = view, i = index, chunk_size = chunk_size, num_threads = index, count = entity_count]() {
            const auto  start = i * chunk_size;
            const auto  end   = (i == num_threads - 1) ? count : (i + 1) * chunk_size;
            const auto& track = perfetto::Track::ThreadScoped((void*) trackSequence.fetch_add(1), TrackPrincipal);
            FREYR_TRACE_BEGIN("USER", "ECS_Iteracao_EnTT_Paralelo", track, "EntityCount", end - start);

            auto it    = view.begin();
            auto endIt = view.end();
            std::advance(it, start);

            for (size_t j = start; j < end && it != endIt; ++j, ++it)
            {
                auto [pos, vel] = view.get(*it);
                pos.x += vel.x;
                pos.y += vel.y;
                pos.y += vel.z;
            }
            FREYR_TRACE_END("USER", track);
        });
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }
}

int main(int argc, char const* argv[])
{
    size_t entity_count = 10'000'000;
    size_t thread_count = 8;

    // EnTT_Iniciar(entity_count);
    Freyr_Iniciar(entity_count, 16 * 3 * 1024, thread_count);

    // app->GetRootServiceProvider()->GetService<fr::Scene>()->BeginProfiling();
    app->GetRootServiceProvider()->GetService<fr::Scene>()->Update(0.0f);

    for (size_t i = 0; i < 1000; i++)
    {
        // ECS_Iteracao_EnTT_Paralelo(entity_count, thread_count);

        ECS_Iteracao_Freyr_RouboTrabalho();
        // app->GetRootServiceProvider()->GetService<fr::Scene>()->EndProfiling();
    }

    return 0;
}
