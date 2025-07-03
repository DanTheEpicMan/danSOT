//I lied, this also has some functions

#ifndef GAMESTRUCTS_H
#define GAMESTRUCTS_H
#include <cmath>
#include <cstring>
#include <cstdint>


class FVector
{
public:
    FVector() : x(0.f), y(0.f), z(0.f) {}
    FVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    ~FVector() {}



    float x;
    float y;
    float z;

    float Size() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    float SizeSquared() const {
        return x * x + y * y + z * z;
    }

    FVector unit() const {
        float length = this->Size();
        if (length > 0.0001f) {
            return FVector(x / length, y / length, z / length);
        }
        return FVector(0.f, 0.f, 0.f);
    }


    // Marked as const because it doesn't modify the 'this' vector
    inline float Dot(FVector v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    // This is just an alias for Dot, also marked const
    float dot(const FVector& other) const {
        return this->x * other.x + this->y * other.y + this->z * other.z;
    }

    // normalize() modifies the vector, so it CANNOT be const. This is correct.
    void normalize() {
        float length = std::sqrt(x * x + y * y + z * z);
        if (length > 0.0001f) {
            x /= length;
            y /= length;
            z /= length;
        }
    }

    // Your static cross product function is correct. We will change how it's called.
    static FVector cross(const FVector& v1, const FVector& v2) {
        return FVector(
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        );
    }

    // Marked as const
    inline float Distance(FVector v) const {
        return float(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0)));
    }

    float Length() const {
        return sqrtf(x * x + y * y + z * z);
    }

    // Marked as const
    FVector operator+(FVector v) const {
        return FVector(x + v.x, y + v.y, z + v.z);
    }

    // Marked as const
    FVector operator-(FVector v) const {
        return FVector(x - v.x, y - v.y, z - v.z);
    }

    // Marked as const
    FVector operator*(float number) const {
        return FVector(x * number, y * number, z * number);
    }

    // Marked as const
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
    uint64_t m_Data = 0;
    uint32_t m_nCount = 0;
    uint32_t m_nMax = 0;
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

    D3DMATRIX operator*(const D3DMATRIX& other) const {
        D3DMATRIX result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = m[i][0] * other.m[0][j] +
                                 m[i][1] * other.m[1][j] +
                                 m[i][2] * other.m[2][j] +
                                 m[i][3] * other.m[3][j];
            }
        }
        return result;
    }
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

// Quaternion structure
struct FQuat
{
    double x;
    double y;
    double z;
    double w;
};

// You'll need a definition for FRotator if you don't have one
// It's just like FVector but often named Pitch, Yaw, Roll
struct FRotator {
    double Pitch;
    double Yaw;
    double Roll;

    FQuat ToQuat() const {
        const float DEG_TO_RAD = M_PI / (180.f);
        const float DIVIDE_BY_2 = DEG_TO_RAD / 2.f;
        float SP, SY, SR;
        float CP, CY, CR;

        SP = sin(Pitch * DIVIDE_BY_2);
        CP = cos(Pitch * DIVIDE_BY_2);
        SY = sin(Yaw * DIVIDE_BY_2);
        CY = cos(Yaw * DIVIDE_BY_2);
        SR = sin(Roll * DIVIDE_BY_2);
        CR = cos(Roll * DIVIDE_BY_2);

        FQuat q;
        q.x = CR * SP * CY - SR * CP * SY;
        q.y = CR * CP * SY + SR * SP * CY;
        q.z = SR * CP * CY - CR * SP * SY;
        q.w = CR * CP * CY + SR * SP * SY;
        return q;
    }
};


struct alignas(16) FTransform {
    FQuat   Rotation;
    FVector Translation;
    char    pad_0001[4];
    FVector Scale3D;
    char    pad_0002[4];

    FVector TransformPosition(const FVector& V) const {
        const FQuat& R = Rotation;
        const FVector& S = Scale3D;
        const FVector& T = Translation;
        const FVector T2(R.x, R.y, R.z);

        // Fixed the way cross() is called
        const FVector Vtemp = FVector::cross(T2, V) * (2.0f * R.w);
        const FVector V_rotated = V + Vtemp + FVector::cross(T2, Vtemp);

        FVector result;
        result.x = V_rotated.x * S.x + T.x;
        result.y = V_rotated.y * S.y + T.y;
        result.z = V_rotated.z * S.z + T.z;

        return result;
    }

    D3DMATRIX ToMatrixWithScale() const {
        D3DMATRIX m;
        m.m[3][0] = Translation.x;
        m.m[3][1] = Translation.y;
        m.m[3][2] = Translation.z;

        float x2 = Rotation.x + Rotation.x;
        float y2 = Rotation.y + Rotation.y;
        float z2 = Rotation.z + Rotation.z;

        float xx2 = Rotation.x * x2;
        float yy2 = Rotation.y * y2;
        float zz2 = Rotation.z * z2;
        m.m[0][0] = (1.0f - (yy2 + zz2)) * Scale3D.x;
        m.m[1][1] = (1.0f - (xx2 + zz2)) * Scale3D.y;
        m.m[2][2] = (1.0f - (xx2 + yy2)) * Scale3D.z;

        float yz2 = Rotation.y * z2;
        float wx2 = Rotation.w * x2;
        m.m[2][1] = (yz2 - wx2) * Scale3D.z;
        m.m[1][2] = (yz2 + wx2) * Scale3D.y;

        float xy2 = Rotation.x * y2;
        float wz2 = Rotation.w * z2;
        m.m[1][0] = (xy2 - wz2) * Scale3D.y;
        m.m[0][1] = (xy2 + wz2) * Scale3D.x;

        float xz2 = Rotation.x * z2;
        float wy2 = Rotation.w * y2;
        m.m[2][0] = (xz2 + wy2) * Scale3D.z;
        m.m[0][2] = (xz2 - wy2) * Scale3D.x;

        m.m[0][3] = 0.0f;
        m.m[1][3] = 0.0f;
        m.m[2][3] = 0.0f;
        m.m[3][3] = 1.0f;

        return m;
    }
};

struct GUID { //used for team/ship ID
    int A, B, C, D;

    bool operator ==(const GUID& other) const {
        return A == other.A && B == other.B && C == other.C && D == other.D;
    }
};

// struct FString : private TArray<wchar_t>
// {
//     std::wstring ToWString() const
//     {
//         if (!IsValid() || m_nCount <= 0)
//             return L"";
//
//         wchar_t* buffer = new wchar_t[m_nCount + 1];
//         memset(buffer, 0, (m_nCount + 1) * sizeof(wchar_t));
//
//         driver::read_physical((PVOID)m_Data, buffer, m_nCount * sizeof(wchar_t));
//
//         std::wstring result(buffer);
//         delete[] buffer;
//         return result;
//     }
//
//     std::string ToString() const
//     {
//         std::wstring ws = ToWString();
//
//         int size_needed = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
//         std::string str(size_needed, 0);
//         WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &str[0], size_needed, NULL, NULL);
//
//         return str;
//     }
// };

#endif //GAMESTRUCTS_H
