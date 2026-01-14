#pragma once
#include <wrl.h>
#include <_Wrapper/BufferManager.h>
using Microsoft::WRL::ComPtr;

class DX12Wrapper{
public:
    DX12Wrapper(const ComPtr<ID3D12Device>& d)
        : device(d), b(device) {}

    ComPtr<ID3D12Device> device;
    BufferManager b;

private:
    HRESULT hr;
};