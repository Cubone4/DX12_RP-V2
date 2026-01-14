#pragma once
#include <cmath>
#include <DirectXMath.h>
using Microsoft::WRL::ComPtr;

inline constexpr float deg2rad = DirectX::XM_PI / 180.0f;

struct Vector3{
    float x = 0, y = 0, z = 0;

    // This could be made into a union in case other Graphic APIs become involved
    // You would be able to add VkBuffer and such
    ComPtr<ID3D12Resource> buffer;

    // Because other APIs are currently not involved, I have not added
    // a flag to indicate what buffer is being used.


    Vector3& operator=(const Vector3& other){
        x = other.x; y = other.y; z = other.z;

        // Update buffer
        // BufferManager
    }

    #pragma region Math Operations
    Vector3 operator+(const Vector3 other){
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    Vector3 operator+=(const Vector3& other){
        x += other.x; y += other.y; z += other.z;
        return *this;
    };

    Vector3 operator*=(const Vector3& other){
        x *= other.x; y *= other.y; z *= other.z;
        return *this;
    };

    Vector3 operator*(const Vector3& other) const {
        return { x * other.x, y * other.y, z * other.z };
    };

    Vector3 operator*(float scale) const {
        return { x * scale, y * scale, z * scale };
    };
    #pragma endregion

    #pragma region Directional Operations
    Vector3 Forward() const {
        return {
            cosf(x) * sinf(y),
            sinf(x),
            cosf(x) * cosf(y)
        };
    }

    Vector3 Right() const {
        Vector3 f = Forward();
        return Vector3{
            f.z, 0.f, -f.x
        }; // simple perpendicular horizontal
    }

    Vector3 Up() const {
        Vector3 f = Forward();
        Vector3 r = Right();
        return Vector3{
            f.y * r.z - f.z * r.y,
            f.z * r.x - f.x * r.z,
            f.x * r.y - f.y * r.x
        };
    }

    Vector3 ForwardDeg() const {
        return {
            cosf(x * deg2rad) * sinf(y * deg2rad),
            sinf(x * deg2rad),
            cosf(x * deg2rad) * cosf(y * deg2rad)
        };
    }
    #pragma endregion

};