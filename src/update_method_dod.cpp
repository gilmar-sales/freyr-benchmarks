#include <benchmark/benchmark.h>

class GameObject
{
  public:
    GameObject()          = default;
    virtual ~GameObject() = default;

    virtual void Start() {};
    virtual void BeforeUpdate() {};
    virtual void Update() {};
    virtual void AfterUpdate() {};
};

class DynamicObject final : public GameObject
{
  public:
    void Update() override
    {
        x += 1.0f;
        y += 1.0f;
        z += 1.0f;
    }

  private:
    float x {}, y {}, z {};
};

struct Position
{
    float x;
    float y;
    float z;
};

static void MetodoUpdate_Virtual_Dinamico(benchmark::State& state)
{
    auto gameObjects = std::vector<GameObject*>();

    for (auto i = 0; i < state.range(); i++)
    {
        gameObjects.push_back(new DynamicObject());
    }

    for (auto _ : state)
    {
        for (const auto gameObject : gameObjects)
        {
            gameObject->Update();
        }
    }

    for (const auto gameObject : gameObjects)
    {
        delete gameObject;
    }
}

static void MetodoUpdate_Virtual_Contiguo(benchmark::State& state)
{
    auto gameObjects = std::vector<GameObject*>();

    auto dynamicObjects = new DynamicObject[state.range()];

    for (auto i = 0; i < state.range(); i++)
    {
        gameObjects.push_back(&dynamicObjects[i]);
    }

    for (auto _ : state)
    {
        for (const auto gameObject : gameObjects)
        {
            gameObject->Update();
        }
        state.SetItemsProcessed(state.range());
    }

    delete[] dynamicObjects;
}

static void MetodoUpdate_Direto_Dinamico(benchmark::State& state)
{
    auto positions = std::vector<Position*>();

    for (auto i = 0; i < state.range(); i++)
    {
        positions.push_back(new Position());
    }

    for (auto _ : state)
    {
        for (auto& position : positions)
        {
            position->x += 1.0f;
            position->y += 1.0f;
            position->z += 1.0f;
        }
        state.SetItemsProcessed(state.range());
    }
}

static void MetodoUpdate_Direto_Contiguo(benchmark::State& state)
{
    auto positions = std::vector<Position>(state.range());

    for (auto _ : state)
    {
        for (auto& position : positions)
        {
            position.x += 1.0f;
            position.y += 1.0f;
            position.z += 1.0f;
        }
        state.SetItemsProcessed(state.range());
    }
}

BENCHMARK(MetodoUpdate_Virtual_Dinamico)
    ->Iterations(1000)
    ->RangeMultiplier(2)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000);

BENCHMARK(MetodoUpdate_Virtual_Contiguo)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000);

BENCHMARK(MetodoUpdate_Direto_Dinamico)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000);

BENCHMARK(MetodoUpdate_Direto_Contiguo)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000);

BENCHMARK_MAIN();
