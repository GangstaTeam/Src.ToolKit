#pragma once

#include <wincodec.h>
#include <windows.h>

namespace tk
{

template <typename T> struct ComPtr
{
    T* ptr = nullptr;
    ComPtr() = default;
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    ~ComPtr()
    {
        if (ptr)
        {
            ptr->Release();
        }
    }

    T** operator&() { return &ptr; }
    T* operator->() const { return ptr; }
};

IWICImagingFactory* WicFactory();

}
