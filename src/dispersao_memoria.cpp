#include <cstdint>
#include <cmath>
#include <iostream>
#include <vector>

class GameObject
{
  public:
    GameObject() = default;

    virtual ~GameObject() = default;

    virtual void Start() {
    };

    virtual void Update() {
    };

    virtual void AfterUpdate() {
    };
};

class DynamicObject final : public GameObject
{
  public:
    DynamicObject() :
        x(0),
        y(0),
        z(0)
    {
    }

    void Update() override
    {
        x += 1.0f;
        y += 1.0f;
        z += 1.0f;
    }

  private:
    float x, y, z;
};

constexpr auto GameObjectCount = 10'000;

int main()
{
    std::cout << "n,deslocamento,x,y" << std::endl;

    auto gameObjects = std::vector<GameObject*>();

    for (int i = 0; i < GameObjectCount; i++)
    {
        gameObjects.push_back(new DynamicObject());
    }

    const auto minAddress = *std::min(gameObjects.begin(), gameObjects.end());

    const auto half = static_cast<std::uint64_t>(std::sqrt(GameObjectCount));

    auto i = 0;
    for (auto gameObject : gameObjects)
    {
        const auto offset = (gameObject - minAddress);
        const auto x      = (gameObject - minAddress) / half;
        const auto y      = (gameObject - minAddress) % half;

        std::cout << i++ << "," << offset << "," << x << "," << y << std::endl;
    }

    return 0;
}
