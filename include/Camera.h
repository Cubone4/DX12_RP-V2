#pragma once
#include <wrl.h>
#include <Engine/Vector3.h>
#include <iostream>
#include <algorithm>
#include <DirectXMath.h>
using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct CameraTransform {
    Vector3 position;
    Vector3 rotation;

    float FOV = 60;
    float nearClippingPlane = 0.1f;
    float farClippingPlane = 100;
};

struct alignas(256) DXCamBuffer{
    XMMATRIX worldMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projMatrix;
};

class Camera{
public:
    CameraTransform camTransform;

    #pragma region DX Functions
    float deg2rad = XM_PI / 180.0f;

    inline XMVECTOR Vec3ToXMVECTOR(const Vector3& vec) const { 
        return XMVectorSet(vec.x, vec.y, vec.z, 1.0f); 
    }

    DXCamBuffer GetDXBuffer() const {
        DXCamBuffer buf;
        
        buf.worldMatrix = XMMatrixTranspose(XMMatrixIdentity());
        
        Vector3 forw = camTransform.rotation.ForwardDeg();
        XMVECTOR forward = XMVectorSet(forw.x,forw.y,forw.z,0.0f);

        buf.viewMatrix = XMMatrixTranspose(
            XMMatrixLookAtLH(Vec3ToXMVECTOR(camTransform.position), 
            XMVectorAdd(Vec3ToXMVECTOR(camTransform.position), forward), 
            XMVectorSet(0, 1, 0, 0) )
        );

        buf.projMatrix  = XMMatrixTranspose(XMMatrixPerspectiveFovLH(
            camTransform.FOV * (XM_PI/180.0f),
            800.0f / 600.0f,
            camTransform.nearClippingPlane,
            camTransform.farClippingPlane
        ));

        return buf;
    }
    
    void UpdateCamBuffer(ComPtr<ID3D12Resource> buf){
        if(constBuffer == nullptr) constBuffer = buf;
        if(!bufMapDest) buf->Map(0, nullptr, &bufMapDest);
        memcpy(bufMapDest, &GetDXBuffer(), sizeof(DXCamBuffer));
    }
    #pragma endregion

    void Move(const Vector3& move, const bool normalized = true){
        if(constBuffer == nullptr) return;

        if(!normalized) camTransform.position += move;
        else {
            camTransform.position += camTransform.rotation.Forward() * move;
        }

        UpdateCamBuffer(constBuffer);
    }





    void Rotate(const Vector3& move){
        if(constBuffer == nullptr) return;
        camTransform.rotation += move;
        UpdateCamBuffer(constBuffer);
    }

    void MouseLook() {
        if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
            firstMouse = true;
            ShowCursor(TRUE);
            return;
        }

        ShowCursor(FALSE);

        RECT rect;
        GetClientRect(hwnd, &rect);

        POINT center{
            (rect.right - rect.left) / 2,
            (rect.bottom - rect.top) / 2
        };

        ClientToScreen(hwnd, &center);

        POINT mouse;
        GetCursorPos(&mouse);

        float dx = float(mouse.x - center.x);
        float dy = float(mouse.y - center.y);

        camTransform.rotation.y += dx * mouseSensitivity;
        camTransform.rotation.x -= dy * mouseSensitivity;

        SetCursorPos(center.x, center.y);

        ViewportMovement();
    }



    void ViewportMovement(){
        if (GetAsyncKeyState('W') & 0x8000) { camTransform.position.z += 0.1f; }
        if (GetAsyncKeyState('A') & 0x8000) { camTransform.position.x -= 0.1f; }
        if (GetAsyncKeyState('S') & 0x8000) { camTransform.position.z -= 0.1f; }
        if (GetAsyncKeyState('D') & 0x8000) { camTransform.position.x += 0.1f; }

        if (GetAsyncKeyState('Q') & 0x8000) { camTransform.position.y -= 0.1f; }
        if (GetAsyncKeyState('E') & 0x8000) { camTransform.position.y += 0.1f; }
        
        UpdateCamBuffer(constBuffer);
    }
    

    ~Camera(){
        if(bufMapDest != nullptr)  bufMapDest = nullptr;
        if(constBuffer != nullptr) constBuffer->Unmap(0, nullptr);
    }

    HWND hwnd;

private:

    void* bufMapDest = nullptr;
    ComPtr<ID3D12Resource> constBuffer = nullptr;

    POINT lastMousePos{};
    bool firstMouse = true;
    float mouseSensitivity = 0.1f;

};