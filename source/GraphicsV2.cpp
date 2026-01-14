#include <windows.h>
#include <iostream>
#include <RenderManager.h>

#define LUM_EDITOR

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"GraphicsV2";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, L"GraphicsV2", L"DX12",
        WS_OVERLAPPEDWINDOW,
        100, 100, 800, 600,
        nullptr, nullptr, hInst, nullptr
    );

    ShowWindow(hwnd, SW_SHOW);
    
    ComPtr<ID3D12Device> device;
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    if (FAILED(hr)) {std::cout << "Error creatin D3D12 Device\n"; system("Pause");}
    RenderManager renderManager(device);
    if (!renderManager.Initialize(hwnd)) { std::cout << "Failed to initialize DX12\n"; system("pause"); return -1; }

    
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        renderManager.Render();
    }

    return 0;
}
