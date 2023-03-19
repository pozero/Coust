#include "pch.h"

#include "Coust/Utils/Random.h"

namespace Coust::Random 
{
    // This random generator could be accessed by multiple threads
    static thread_local std::mt19937 s_RNG{};
    
    void Seed()
    {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto seed = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
        s_RNG.seed((unsigned int) seed);
    }
    
    int UInteger(int i, int j)
    {
        if (i > j)
            std::swap(i, j);

        std::uniform_int_distribution<int> dist(i, j);
        return dist(s_RNG);
    }
    
    float UFloat(float i, float j)
    {
        if (i > j)
            std::swap(i, j);
        
        std::uniform_real_distribution<float> dist(i, j);
        return dist(s_RNG);
    }
}