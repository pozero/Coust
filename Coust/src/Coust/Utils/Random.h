#pragma once

#include <vector>

namespace Coust::Random
{
    void Seed();

    int UInteger(int i, int j);

    float UFloat(float i, float j);
    
    template<typename T>
    const T& Pick(const std::vector<T>& vec)
    {
        int index = UInteger(0, (int) vec.size() - 1);
        return vec[index];
    }
    
    template<typename T>
    T& Pick(std::vector<T>& vec)
    {
        return const_cast<T&>(Pick(const_cast<const std::vector<T>&>(vec)));
    }
    
    template<typename T>
    const T* Pick(const T* ptr, size_t size)
    {
        int index = UInteger(0, (int) size - 1);
        return ptr + index;
    }
    
    template<typename T>
    T* Pick(T* ptr, size_t size)
    {
        return const_cast<T*>(Pick(const_cast<const T*>(ptr), size));
    }
}