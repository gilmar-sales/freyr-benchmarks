#include <fcntl.h>

#include <chrono>
#include <thread>

#include <flecs.h>
#include <perfetto.h>

namespace perfetto
{
    class TracingSession;
}

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("flecs").SetDescription("Application"));

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

struct Position
{
    float x;
    float y;
    float z;
};

struct Velocity
{
    float x;
    float y;
    float z;
};

struct Acceleration
{
    float x;
    float y;
    float z;
};

thread_local float deltaTime = 0.016f;

static void ApplyGravitySystem(Acceleration& acceleration)
{
    acceleration.y -= 9.84f * deltaTime;
}

static void ApplyAccelerationSystem(Velocity& velocity, Acceleration& acceleration)
{
    velocity.x += acceleration.x * deltaTime;
    velocity.y += acceleration.y * deltaTime;
    velocity.y += acceleration.y * deltaTime;
}

static void ApplyVelocitySystem(Position& position, Velocity& velocity)
{

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z += velocity.z * deltaTime;
}

int main()
{
    ecs_os_api.perf_trace_push_ = [](const char* name, auto, auto) {
        TRACE_EVENT_BEGIN("flecs", perfetto::DynamicString{name});
    };

    ecs_os_api.perf_trace_pop_ = [](const char* name, auto, auto) {
        TRACE_EVENT_END("flecs");
    };

    auto args = perfetto::TracingInitArgs();
    args.backends |= perfetto::kInProcessBackend;
    args.shmem_size_hint_kb = 32 * 1024;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();

    perfetto::TraceConfig cfg;
    cfg.set_write_into_file(true);     // Stream to disk, not RAM
    cfg.set_file_write_period_ms(100); // Drain buffer every 500ms
    cfg.set_flush_period_ms(1000);     // Flush SMB every 10s to avoid out-of-order drops
    cfg.mutable_incremental_state_config()->set_clear_period_ms(500);

    auto* buf = cfg.add_buffers();
    buf->set_size_kb(512 * 1024);
    buf->set_fill_policy(
        perfetto::TraceConfig::BufferConfig::RING_BUFFER);

    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    perfetto::protos::gen::TrackEventConfig te_cfg;
    te_cfg.add_enabled_categories("flecs");
    te_cfg.set_disable_incremental_timestamps(false);
    ds_cfg->set_track_event_config_raw(te_cfg.SerializeAsString()); // ← fix the bug here too

    auto       session = perfetto::Tracing::NewTrace();
    const auto output  = std::format("flecs_trace_{}.pftrace", std::chrono::system_clock::now().time_since_epoch().count());
    int        fd      = open(output.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
    session->Setup(cfg, fd);
    session->StartBlocking();

    flecs::world* flecsWorld = new flecs::world;
    flecsWorld->set_threads(std::thread::hardware_concurrency() - 1);

    auto MakeTracedRun = [](const char* name) {
        return [name](flecs::iter& it) {
            TRACE_EVENT_BEGIN("flecs", perfetto::DynamicString{name});
            while (it.next()) {
                it.each();
            }
            TRACE_EVENT_END("flecs");
        };
    };

    flecs::system gravitySystem =
        flecsWorld->system<Acceleration>()
            .multi_threaded()
            .run(MakeTracedRun("ApplyGravity"), ApplyGravitySystem);

    flecs::system accelerationSystem =
        flecsWorld->system<Velocity, Acceleration>()
            .multi_threaded()
            .run(MakeTracedRun("ApplyAcceleration"), ApplyAccelerationSystem);

    flecs::system velocitySystem =
        flecsWorld->system<Position, Velocity>()
            .multi_threaded()
            .run(MakeTracedRun("ApplyVelocity"), ApplyVelocitySystem);

    flecsWorld->component<Position>();
    flecsWorld->component<Velocity>();
    flecsWorld->component<Acceleration>();

    for (auto i = 0u; i < 1'000'000; ++i)
    {
        flecsWorld->entity()
            .add<Position>()
            .add<Velocity>()
            .set<Acceleration>({ .x = 1.0f, .y = 1.0f, .z = 0.0f });
    }

    for (int i = 0; i < 1000; ++i)
    {
        TRACE_EVENT("flecs", "Update");
        flecsWorld->progress();
    }

    session->StopBlocking();
    close(fd);
}
