//I lied, this also has some functions

#ifndef GAMESTRUCTS_H
#define GAMESTRUCTS_H
#include <cmath>

class FVector
{
public:
    FVector() : x(0.f), y(0.f), z(0.f)
    {

    }

    FVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
    {

    }
    ~FVector()
    {

    }

    float x;
    float y;
    float z;

    inline float Dot(FVector v)
    {
        return x * v.x + y * v.y + z * v.z;
    }


    float dot(const FVector& other) const
    {
        return this->x * other.x + this->y * other.y + this->z * other.z;
    }


    void normalize() {
        float length = std::sqrt(x * x + y * y + z * z);
        if (length > 0.0001f) {
            x /= length;
            y /= length;
            z /= length;
        }
    }

    // Cross product of two vectors
    static FVector cross(const FVector& v1, const FVector& v2) {
        return FVector(
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        );
    }

    inline float Distance(FVector v)
    {
        return float(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0)));
    }

    FVector operator+(FVector v)
    {
        return FVector(x + v.x, y + v.y, z + v.z);
    }

    FVector operator-(FVector v)
    {
        return FVector(x - v.x, y - v.y, z - v.z);
    }

    FVector operator*(float number) const {
        return FVector(x * number, y * number, z * number);
    }

    FVector operator/(float number) const {
        return FVector(x / number, y / number, z / number);
    }
};

struct FMinimalViewInfo
{
    FVector Location; // 0x0(0x0)
    FVector Rotation; // 0xC(0xC)

    char pad_4[0x10];
    float FOV; // 0x18(0x18)

    float DesiredFOV; // 0x1c(0x04)
    float OrthoWidth; // 0x20(0x04)
    float OrthoNearClipPlane; // 0x24(0x04)
    float OrthoFarClipPlane; // 0x28(0x04)
    float AspectRatio; // 0x2c(0x04)
};

struct FCameraCacheEntry
{
    float Timestamp; // 0x00(0x04)
    char pad_4[0x10 - 0x04]; // 0x04(0x0c)
    FMinimalViewInfo POV; // 0x10(0x10)
};

template<class T>
class TArray
{
public:
    int Length() const
    {
        return m_nCount;
    }

    bool IsValid() const
    {
        if (m_nCount > m_nMax)
            return false;
        if (!m_Data)
            return false;
        return true;
    }

    uint64_t GetAddress() const
    {
        return m_Data;
    }

protected:
    uint64_t m_Data;
    uint32_t m_nCount;
    uint32_t m_nMax;
};



// Matrix structure definition
struct D3DMATRIX {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
};

inline D3DMATRIX Matrix(FVector rot, FVector origin) {
    float radPitch = (rot.x * M_PI / 180.f);
    float radYaw = (rot.y * M_PI / 180.f);
    float radRoll = (rot.z * M_PI / 180.f);

    float SP = sinf(radPitch), CP = cosf(radPitch);
    float SY = sinf(radYaw), CY = cosf(radYaw);
    float SR = sinf(radRoll), CR = cosf(radRoll);

    D3DMATRIX matrix;
    matrix.m[0][0] = CP * CY;
    matrix.m[0][1] = CP * SY;
    matrix.m[0][2] = SP;
    matrix.m[0][3] = 0.f;

    matrix.m[1][0] = SR * SP * CY - CR * SY;
    matrix.m[1][1] = SR * SP * SY + CR * CY;
    matrix.m[1][2] = -SR * CP;
    matrix.m[1][3] = 0.f;

    matrix.m[2][0] = -(CR * SP * CY + SR * SY);
    matrix.m[2][1] = CY * SR - CR * SP * SY;
    matrix.m[2][2] = CR * CP;
    matrix.m[2][3] = 0.f;

    matrix.m[3][0] = origin.x;
    matrix.m[3][1] = origin.y;
    matrix.m[3][2] = origin.z;
    matrix.m[3][3] = 1.f;

    return matrix;
}

#endif //GAMESTRUCTS_H
