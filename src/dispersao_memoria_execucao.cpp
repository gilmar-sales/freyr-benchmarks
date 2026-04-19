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

static void Alocacao_Dinamica_Execucao(benchmark::State& state)
{
    auto gameObjects = std::vector<DynamicObject*>();

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

static void Alocacao_Contigua_Execucao(benchmark::State& state)
{
    auto gameObjects = std::vector<DynamicObject*>();
    gameObjects.reserve(state.range());

    auto allocs = new DynamicObject[state.range()];
    for (auto i = 0; i < state.range(); i++)
    {
        gameObjects.push_back(&allocs[i]);
    }

    for (auto _ : state)
    {
        for (const auto gameObject : gameObjects)
        {
            gameObject->Update();
        }
    }

    delete[] allocs;
}

BENCHMARK(Alocacao_Dinamica_Execucao)->Arg(10'000);
BENCHMARK(Alocacao_Contigua_Execucao)->Arg(10'000);

BENCHMARK_MAIN();
