#include "pch.h"
#include "Global/Quaternion.h"

static float NormalizeAxis(float Angle)
{
	Angle = fmodf(Angle, 360.f);
	if (Angle > 180.f)
	{
		Angle -= 360.f;
	}
	else if (Angle < -180.f)
	{
		Angle += 360.f;
	}
	return Angle;
}

FQuaternion FQuaternion::FromAxisAngle(const FVector& Axis, float AngleRad)
{
	FVector N = Axis;
	N.Normalize();
	float s = sinf(AngleRad * 0.5f);
	return FQuaternion(
		N.X * s,
		N.Y * s,
		N.Z * s,
		cosf(AngleRad * 0.5f)
	);
}

FQuaternion FQuaternion::FromEuler(const FVector& EulerDeg)
{
	// EulerDeg: (X=Roll, Y=Pitch, Z=Yaw) in degrees
	// Unreal Engine Standard: FRotator(Pitch, Yaw, Roll) → FQuat
	FVector Radians = FVector::GetDegreeToRadian(EulerDeg);

	float SR = sinf(Radians.X * 0.5f); // Roll
	float CR = cosf(Radians.X * 0.5f);
	float SP = sinf(Radians.Y * 0.5f); // Pitch
	float CP = cosf(Radians.Y * 0.5f);
	float SY = sinf(Radians.Z * 0.5f); // Yaw
	float CY = cosf(Radians.Z * 0.5f);

	// Unreal Engine Standard Formula
	return {
		 CR * SP * SY - SR * CP * CY, // X
		-CR * SP * CY - SR * CP * SY, // Y
		 CR * CP * SY - SR * SP * CY, // Z
		 CR * CP * CY + SR * SP * SY  // W
	};
}

FQuaternion FQuaternion::FromRotationMatrix(const FMatrix& M)
{
    // UE Standard: TQuat<T>::TQuat(const TMatrix<T>& M)
    // Reference: UnrealEngine/Engine/Source/Runtime/Core/Public/Math/Quat.h (line 777-849)
    FQuaternion Q;

    // Check diagonal (trace)
    const float tr = M.Data[0][0] + M.Data[1][1] + M.Data[2][2];

    if (tr > 0.0f)
    {
        // Trace > 0: Standard case
        float InvS = 1.0f / sqrtf(tr + 1.0f);
        Q.W = 0.5f * (1.0f / InvS);
        float s = 0.5f * InvS;

        Q.X = (M.Data[1][2] - M.Data[2][1]) * s;
        Q.Y = (M.Data[2][0] - M.Data[0][2]) * s;
        Q.Z = (M.Data[0][1] - M.Data[1][0]) * s;
    }
    else
    {
        // Trace <= 0: Find largest diagonal element
        int i = 0;
        if (M.Data[1][1] > M.Data[0][0])
        {
            i = 1;
        }
        if (M.Data[2][2] > M.Data[i][i])
        {
            i = 2;
        }

        static constexpr int nxt[3] = { 1, 2, 0 };
        const int j = nxt[i];
        const int k = nxt[j];

        float s = M.Data[i][i] - M.Data[j][j] - M.Data[k][k] + 1.0f;
        float InvS = 1.0f / sqrtf(s);

        float qt[4];
        qt[i] = 0.5f * (1.0f / InvS);

        s = 0.5f * InvS;

        qt[3] = (M.Data[j][k] - M.Data[k][j]) * s;
        qt[j] = (M.Data[i][j] + M.Data[j][i]) * s;
        qt[k] = (M.Data[i][k] + M.Data[k][i]) * s;

        Q.X = qt[0];
        Q.Y = qt[1];
        Q.Z = qt[2];
        Q.W = qt[3];
    }

    return Q;
}

FVector FQuaternion::ToEuler() const
{
	// UE Standard conversion: Quaternion → (Roll, Pitch, Yaw)
	// Reference: UE5 UnrealMath.cpp FQuat4f::Rotator()
	FVector Euler;

	// Gimbal Lock singularity detection (Pitch ±90°)
	const float SingularityTest = Z * X - W * Y;
	const float YawY = 2.f * (W * Z + X * Y);
	const float YawX = (1.f - 2.f * (Y * Y + Z * Z));

	constexpr float SINGULARITY_THRESHOLD = 0.4999995f;
	constexpr float RAD_TO_DEG = (180.f) / PI;

	if (SingularityTest < -SINGULARITY_THRESHOLD) // South pole singularity
	{
		Euler.Y = -90.f; // Pitch = -90°
		Euler.Z = atan2f(YawY, YawX) * RAD_TO_DEG;
		Euler.X = NormalizeAxis(-Euler.Z - (2.f * atan2f(X, W) * RAD_TO_DEG));
	}
	else if (SingularityTest > SINGULARITY_THRESHOLD) // North pole singularity
	{
		Euler.Y = 90.f; // Pitch = 90°
		Euler.Z = atan2f(YawY, YawX) * RAD_TO_DEG;
		Euler.X = NormalizeAxis(Euler.Z - (2.f * atan2f(X, W) * RAD_TO_DEG));
	}
	else // Normal case
	{
		Euler.Y = asinf(2.f * SingularityTest) * RAD_TO_DEG; // Pitch
		Euler.Z = atan2f(YawY, YawX) * RAD_TO_DEG; // Yaw
		Euler.X = atan2f(-2.f * (W * X + Y * Z), (1.f - 2.f * (X * X + Y * Y))) * RAD_TO_DEG; // Roll
	}

	return Euler;
}

FMatrix FQuaternion::ToRotationMatrix() const
{
    FMatrix M;

    const float X2 = X * X;
    const float Y2 = Y * Y;
    const float Z2 = Z * Z;
    const float XY = X * Y;
    const float XZ = X * Z;
    const float YZ = Y * Z;
    const float WX = W * X;
    const float WY = W * Y;
    const float WZ = W * Z;

    M.Data[0][0] = 1.0f - 2.0f * (Y2 + Z2);
    M.Data[0][1] = 2.0f * (XY + WZ);
    M.Data[0][2] = 2.0f * (XZ - WY);
    M.Data[0][3] = 0.0f;

    M.Data[1][0] = 2.0f * (XY - WZ);
    M.Data[1][1] = 1.0f - 2.0f * (X2 + Z2);
    M.Data[1][2] = 2.0f * (YZ + WX);
    M.Data[1][3] = 0.0f;

    M.Data[2][0] = 2.0f * (XZ + WY);
    M.Data[2][1] = 2.0f * (YZ - WX);
    M.Data[2][2] = 1.0f - 2.0f * (X2 + Y2);
    M.Data[2][3] = 0.0f;

    M.Data[3][0] = 0.0f;
    M.Data[3][1] = 0.0f;
    M.Data[3][2] = 0.0f;
    M.Data[3][3] = 1.0f;

    return M;
}

FQuaternion FQuaternion::operator*(const FQuaternion& Q) const
{
	return {
		W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y,
		W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X,
		W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
		W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z
	};
}

void FQuaternion::Normalize()
{
	float mag = sqrtf(X * X + Y * Y + Z * Z + W * W);
	if (mag > 0.0001f)
	{
		X /= mag;
		Y /= mag;
		Z /= mag;
		W /= mag;
	}
}

FQuaternion FQuaternion::MakeFromDirection(const FVector& Direction)
{
	const FVector& ForwardVector = FVector::ForwardVector();
	FVector Dir = Direction.GetNormalized();

	float Dot = ForwardVector.Dot(Dir);
	if (Dot == 1.f)
	{
		return Identity();
	}

	if (Dot == -1.f)
	{
		FVector RotAxis = ForwardVector.Cross(FVector::UpVector());
		if (RotAxis.IsZero()) // Forward가 UP 벡터와 평행하면 Right 벡터 사용
		{
			RotAxis = FVector::RightVector();
		}
		return FromAxisAngle(RotAxis.GetNormalized(), PI);
	}

	float AngleRad = acos(Dot);

	// ForwardVector를 Dir로 회전시키는 회전축 계산
	FVector Axis = ForwardVector.Cross(Dir);
	Axis.Normalize();
	return FromAxisAngle(Axis, AngleRad);
}

FVector FQuaternion::RotateVector(const FQuaternion& q, const FVector& v)
{
	FQuaternion p(v.X, v.Y, v.Z, 0.0f);
	FQuaternion r = q * p * q.Inverse();
	return { r.X, r.Y, r.Z };
}

FVector FQuaternion::RotateVector(const FVector& V) const
{
	const FVector Q(X, Y, Z);
	const FVector T = 2.f * Q.Cross(V);
	const FVector Result = V + (W * T) + Q.Cross(T);
	return Result;
}
