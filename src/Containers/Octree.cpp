#include "Octree.hpp"

#include <glm/ext/matrix_transform.hpp>

void Octree::Remove(fr::Entity entity)
{
    for (auto i = 0; i < mElements.size(); i++)
    {
        if (auto element = mElements.get(i); element.has_value() && element->entity == entity)
            mElements.remove(i);
    }
}

Octree::Octree(const glm::vec3 position, const float halfRange, ArenaAllocator* allocator, Octree* root) :
    mAllocator(allocator), mPosition(position), mHalfRange(halfRange), mRoot(root)
{
    if (mRoot == nullptr)
        mRoot = this;

    mNearTopLeft = nullptr;
}

bool Octree::Contains(const Particle& particle) const
{
    return (particle.transform.position.x >= mPosition.x - mHalfRange &&
            particle.transform.position.x <= mPosition.x + mHalfRange &&
            particle.transform.position.y >= mPosition.y - mHalfRange &&
            particle.transform.position.y <= mPosition.y + mHalfRange &&
            particle.transform.position.z >= mPosition.z - mHalfRange &&
            particle.transform.position.z <= mPosition.z + mHalfRange);
}

Octree* Octree::Insert(const Particle& particle)
{
    if (!Contains(particle))
    {
        return nullptr;
    }

    while (true)
    {

        if (State current_state = mState.load(std::memory_order_acquire); current_state == State::Leaf)
        {
            if (mElements.push(particle))
            {
                return this;
            }

            TrySubdivide();
        }
        else if (current_state == State::Branch)
        {
            auto ptr = mNearTopLeft->Insert(particle);

            if (ptr)
                return ptr;

            ptr = mNearTopRight->Insert(particle);

            if (ptr)
                return ptr;

            ptr = mNearBotLeft->Insert(particle);

            if (ptr)
                return ptr;

            ptr = mNearBotRight->Insert(particle);

            if (ptr)
                return ptr;

            ptr = mFarTopLeft->Insert(particle);

            if (ptr)
                return ptr;

            ptr = mFarTopRight->Insert(particle);

            if (ptr)
                return ptr;

            ptr = mFarBotLeft->Insert(particle);

            if (ptr)
                return ptr;

            ptr = mFarBotRight->Insert(particle);

            if (ptr)
                return ptr;

            return nullptr;
        }
    }
}

void Octree::TrySubdivide()
{
    State expected = State::Leaf;

    if (!mState.compare_exchange_strong(expected, State::Subdividing, std::memory_order_acquire))
    {
        return;
    }

    float halfRange = mHalfRange / 2;

    glm::vec3 nearTopLeftPos  = { mPosition.x - halfRange, mPosition.y - halfRange, mPosition.z - halfRange };
    glm::vec3 nearTopRightPos = { mPosition.x + halfRange, mPosition.y - halfRange, mPosition.z - halfRange };
    glm::vec3 nearBotLeftPos  = { mPosition.x - halfRange, mPosition.y + halfRange, mPosition.z - halfRange };
    glm::vec3 nearBotRightPos = { mPosition.x + halfRange, mPosition.y + halfRange, mPosition.z - halfRange };

    mNearTopLeft  = mAllocator->construct<Octree>(nearTopLeftPos, halfRange, mAllocator, mRoot);
    mNearTopRight = mAllocator->construct<Octree>(nearTopRightPos, halfRange, mAllocator, mRoot);
    mNearBotLeft  = mAllocator->construct<Octree>(nearBotLeftPos, halfRange, mAllocator, mRoot);
    mNearBotRight = mAllocator->construct<Octree>(nearBotRightPos, halfRange, mAllocator, mRoot);

    glm::vec3 farTopLeftPos  = { mPosition.x - halfRange, mPosition.y - halfRange, mPosition.z + halfRange };
    glm::vec3 farTopRightPos = { mPosition.x + halfRange, mPosition.y - halfRange, mPosition.z + halfRange };
    glm::vec3 farBotLeftPos  = { mPosition.x - halfRange, mPosition.y + halfRange, mPosition.z + halfRange };
    glm::vec3 farBotRightPos = { mPosition.x + halfRange, mPosition.y + halfRange, mPosition.z + halfRange };

    mFarTopLeft  = mAllocator->construct<Octree>(farTopLeftPos, halfRange, mAllocator, mRoot);
    mFarTopRight = mAllocator->construct<Octree>(farTopRightPos, halfRange, mAllocator, mRoot);
    mFarBotLeft  = mAllocator->construct<Octree>(farBotLeftPos, halfRange, mAllocator, mRoot);
    mFarBotRight = mAllocator->construct<Octree>(farBotRightPos, halfRange, mAllocator, mRoot);

    mState.store(State::Branch, std::memory_order_release);
}

void Octree::Query(Particle& particle, std::vector<Particle>& found)
{
    if (!Intersect(particle))
    {
        return;
    }

    for (auto i = 0; i < mElements.size(); i++)
    {
        auto other = mElements.get(i);

        if (!other.has_value() || particle.entity == other->entity)
            continue;

        if (particle.Intersect(*other))
        {
            found.push_back(*other);
        }
    }

    if (mNearTopLeft)
    {
        mNearTopLeft->Query(particle, found);
        mNearTopRight->Query(particle, found);
        mNearBotLeft->Query(particle, found);
        mNearBotRight->Query(particle, found);
        mFarTopLeft->Query(particle, found);
        mFarTopRight->Query(particle, found);
        mFarBotLeft->Query(particle, found);
        mFarBotRight->Query(particle, found);
    }
}

bool Octree::Intersect(const Particle& particle) const
{
    return (particle.transform.position.x >= mPosition.x - (mHalfRange + particle.sphereCollider.radius) &&
            particle.transform.position.x <= mPosition.x + (mHalfRange + particle.sphereCollider.radius) &&
            particle.transform.position.y >= mPosition.y - (mHalfRange + particle.sphereCollider.radius) &&
            particle.transform.position.y <= mPosition.y + (mHalfRange + particle.sphereCollider.radius) &&
            particle.transform.position.z >= mPosition.z - (mHalfRange + particle.sphereCollider.radius) &&
            particle.transform.position.z <= mPosition.z + (mHalfRange + particle.sphereCollider.radius));
}
