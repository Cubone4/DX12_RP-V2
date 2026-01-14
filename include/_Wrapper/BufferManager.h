#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <typeinfo>
#include <vector>
using Microsoft::WRL::ComPtr;


class BufferManager{
    public:
    BufferManager(const ComPtr<ID3D12Device>& d) : device(d) {}
    
    template<typename T>
    size_t inline GetBufferSize(const std::vector<T>& vec) {
        return vec.size() * sizeof(T);
    }
    
    template<typename T>
    bool inline UploadToGPU(ComPtr<ID3D12Resource> buffer, D3D12_VERTEX_BUFFER_VIEW* buffView, const std::vector<T>& data){
        if(buffer == nullptr) { std::cout << "NO BUFFER PROVIDED FOR UPLOAD\n"; return false; }
    
        std::cout << "Uploading " << typeid(T).name() << " to GPU\n";
        const UINT bufferSize = GetBufferSize(data);
    
        void* dest;
        buffer->Map(0, nullptr, &dest);
        memcpy(dest, data.data(), bufferSize);
        buffer->Unmap(0, nullptr);
    
        buffView->BufferLocation = buffer->GetGPUVirtualAddress();
        buffView->StrideInBytes = sizeof(Vertex);
        buffView->SizeInBytes = bufferSize;
    
        std::cout << "Data successfully uploaded to " << buffer->GetGPUVirtualAddress() << "\n";
    
        return true;
    }
    
    template<typename T>
    HRESULT inline CreateBuffer(CD3DX12_HEAP_PROPERTIES prop, const std::vector<T>& data, ComPtr<ID3D12Resource>& resource){
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(GetBufferSize(data));
        
        HRESULT hr = device->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&resource)
        );
    
        if(FAILED(hr)) {std::cout << "Failed Creating committed resource for " << typeid(T).name() << "\n"; system("Pause"); return hr; }
        else {std::cout << "Successfully created resource for " << typeid(T).name() << "\n";}
    
        return hr;
    }
    
    template<typename T>
    HRESULT inline CreateBuffer(CD3DX12_HEAP_PROPERTIES prop, const T& data, ComPtr<ID3D12Resource>& resource){
    
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(data));
        
        HRESULT hr = device->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&resource)
        );
    
        if(FAILED(hr)) {std::cout << "Failed Creating committed resource for " << typeid(T).name() << "\n"; system("Pause"); return hr; }
        else {std::cout << "Successfully created resource for " << typeid(T).name() << "\n";}
    
        return hr;
    }
private:
    ComPtr<ID3D12Device> device;
};