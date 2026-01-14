#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <Engine/Vertex.h>
#include <vector>
#include <Camera.h>
#include <functional>
#include <_Wrapper/DX12Wrapper.h>

using Microsoft::WRL::ComPtr;

class RenderManager
{
public:
    bool Initialize(HWND hwnd);
    void Render();

    RenderManager(ComPtr<ID3D12Device> dev) : w(dev), device(dev) {}
    
    void AddUpdateCallback(std::function<void()> func){
        updateCallback.push_back(func);
    }

    Camera cam = Camera();
    
private:
    UINT width = 1920;
    UINT height = 1080;

    std::vector<std::function<void()>> updateCallback;

    static const UINT FrameCount = 2;
    std::vector<Vertex> vertices;
    
    
    DX12Wrapper w;
    
    ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    
    ComPtr<ID3D12Resource> depthBuffer;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    
    ComPtr<ID3D12Resource> constBuffer;
    ComPtr<ID3D12DescriptorHeap> cbvHeap;
    
    HRESULT hr;
    
    // Commands & Device
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<IDXGISwapChain3> swapChain;
    
    // viewport & scissor
    D3D12_VIEWPORT viewport = {};
    D3D12_RECT scissor = {};

    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12Resource> renderTargets[FrameCount];
    UINT rtvDescriptorSize = 0;
    
    
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    
    // Pipeline State Object
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> errorBlob;
    ComPtr<ID3D12PipelineState> pipelineState;

    // Synchronization
    ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue = 0;
    HANDLE fenceEvent = nullptr;

    // Frame tracking
    UINT frameIndex = 0;
};
