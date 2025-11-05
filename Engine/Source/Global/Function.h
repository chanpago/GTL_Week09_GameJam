#pragma once

using std::string;
using std::wstring;

// ============================================================================
// Math Utility Functions
// ============================================================================

/**
 * @brief 값을 min과 max 사이로 제한
 * @tparam T 제한할 타입
 * @param Value 제한할 값
 * @param Min 최소값
 * @param Max 최대값
 * @return 제한된 값
 */
template <typename T>
T Clamp(const T& Value, const T& Min, const T& Max)
{
	return (Value < Min) ? Min : (Value > Max) ? Max : Value;
}

/**
 * @brief 선형 보간 (Linear Interpolation)
 * @tparam T 보간할 타입 (float, FVector, FQuaternion 등)
 * @param A 시작 값
 * @param B 끝 값
 * @param Alpha 보간 비율 [0, 1]
 * @return 보간된 값
 */
template <typename T>
T Lerp(const T& A, const T& B, float Alpha)
{
	return A * (1.0f - Alpha) + B * Alpha;
}

/**
 * @brief 카메라 트랜지션에 사용되는 Easing 타입
 */
enum class ECameraEaseType : uint8
{
	Linear,       // 선형 보간
	EaseIn,       // 천천히 시작
	EaseOut,      // 천천히 끝
	EaseInOut,    // 천천히 시작하고 끝
	SmoothStep,   // 부드러운 시작과 끝
	SmootherStep, // 더 부드러운 시작과 끝
	Bezier        // 베지어 곡선 (커스텀 컨트롤 포인트 사용)
};

namespace CameraEasing
{
	/**
	 * @brief Cubic Bezier curve X coordinate at parameter t
	 */
	inline float BezierX(float t, const float P[4])
	{
		float oneMinusT = 1.0f - t;
		float oneMinusT2 = oneMinusT * oneMinusT;
		float t2 = t * t;
		// X(t) = 3(1-t)²t * P1.x + 3(1-t)t² * P2.x + t³
		return 3.0f * oneMinusT2 * t * P[0] +
			   3.0f * oneMinusT * t2 * P[2] +
			   t2 * t;
	}

	/**
	 * @brief Cubic Bezier curve Y coordinate at parameter t
	 */
	inline float BezierY(float t, const float P[4])
	{
		float oneMinusT = 1.0f - t;
		float oneMinusT2 = oneMinusT * oneMinusT;
		float t2 = t * t;
		// Y(t) = 3(1-t)²t * P1.y + 3(1-t)t² * P2.y + t³
		return 3.0f * oneMinusT2 * t * P[1] +
			   3.0f * oneMinusT * t2 * P[3] +
			   t2 * t;
	}

	/**
	 * @brief Evaluate cubic Bezier curve at time x (X coordinate)
	 * @param x Time value [0, 1] (X coordinate)
	 * @param P Control points [4]: P1.x, P1.y, P2.x, P2.y (P0=(0,0), P3=(1,1) are fixed)
	 * @return Y value at time x
	 */
	inline float EvaluateBezier(float x, const float P[4])
	{
		x = Clamp(x, 0.0f, 1.0f);
		if (x <= 0.0f) return 0.0f;
		if (x >= 1.0f) return 1.0f;

		// Newton-Raphson method to find t such that BezierX(t) = x
		float t = x;
		const int maxIterations = 8;
		const float epsilon = 0.001f;

		for (int i = 0; i < maxIterations; i++)
		{
			float currentX = BezierX(t, P);
			float diff = currentX - x;

			if (fabsf(diff) < epsilon)
				break;

			// Derivative of X(t)
			float oneMinusT = 1.0f - t;
			float dxdt = 3.0f * oneMinusT * oneMinusT * P[0] +
						 6.0f * oneMinusT * t * (P[2] - P[0]) +
						 3.0f * t * t * (1.0f - P[2]);

			if (fabsf(dxdt) < 0.000001f)
				break;

			t = t - diff / dxdt;
			t = Clamp(t, 0.0f, 1.0f);
		}

		return BezierY(t, P);
	}
} // namespace CameraEasing

/**
 * @brief Easing 함수 적용
 * @param Alpha 원본 알파 값 [0, 1]
 * @param EaseType Easing 타입
 * @param BezierControlPoints 베지어 컨트롤 포인트 [4] (Bezier 타입일 때만 사용)
 * @return Easing이 적용된 알파 값
 */
inline float ApplyEasing(float Alpha, ECameraEaseType EaseType, const float* BezierControlPoints = nullptr)
{
	Alpha = Clamp(Alpha, 0.0f, 1.0f);

	switch (EaseType)
	{
	case ECameraEaseType::Linear:
		return Alpha;

	case ECameraEaseType::EaseIn:
		// Quadratic ease in
		return Alpha * Alpha;

	case ECameraEaseType::EaseOut:
		// Quadratic ease out
		return Alpha * (2.0f - Alpha);

	case ECameraEaseType::EaseInOut:
		// Quadratic ease in-out
		if (Alpha < 0.5f)
			return 2.0f * Alpha * Alpha;
		else
			return 1.0f - pow(-2.0f * Alpha + 2.0f, 2.0f) / 2.0f;

	case ECameraEaseType::SmoothStep:
		// Cubic smooth step
		return Alpha * Alpha * (3.0f - 2.0f * Alpha);

	case ECameraEaseType::SmootherStep:
		// Quintic smooth step
		return Alpha * Alpha * Alpha * (Alpha * (Alpha * 6.0f - 15.0f) + 10.0f);

	case ECameraEaseType::Bezier:
		// Cubic Bezier curve with custom control points
		if (BezierControlPoints != nullptr)
		{
			return CameraEasing::EvaluateBezier(Alpha, BezierControlPoints);
		}
		else
		{
			// Fallback to default easeOutQuad if no control points provided
			static const float DefaultBezier[4] = { 0.250f, 0.460f, 0.450f, 0.940f };
			return CameraEasing::EvaluateBezier(Alpha, DefaultBezier);
		}

	default:
		return Alpha;
	}
}

// ============================================================================
// Memory & COM Utilities
// ============================================================================

template <typename T>
static void SafeDelete(T& InDynamicObject)
{
	delete InDynamicObject;
	InDynamicObject = nullptr;
}

template <typename T>
void SafeRelease(T*& ComObj)
{
	if (ComObj != nullptr)
	{
		ComObj->Release();
		ComObj = nullptr;
	}
}

/**
 * @brief wstring을 멀티바이트 FString으로 변환합니다.
 * @param InString 변환할 FString
 * @return 변환된 wstring
 */
static FString WideStringToString(const wstring& InString)
{
	int32 ByteNumber = WideCharToMultiByte(CP_UTF8, 0,
	                                     InString.c_str(), -1, nullptr, 0, nullptr, nullptr);

	FString OutString(ByteNumber, 0);

	WideCharToMultiByte(CP_UTF8, 0,
	                    InString.c_str(), -1, OutString.data(), ByteNumber, nullptr, nullptr);

	return OutString;
}

/**
 * @brief 멀티바이트 UTF-8 FString을 와이드 스트링(wstring)으로 변환합니다.
 * @param InString 변환할 FString (UTF-8)
 * @return 변환된 wstring
 */
static wstring StringToWideString(const FString& InString)
{
	// 필요한 와이드 문자의 개수를 계산
	int32 WideCharNumber = MultiByteToWideChar(CP_UTF8, 0,
										  InString.c_str(), -1, nullptr, 0);

	// 계산된 크기만큼 wstring 버퍼를 할당
	wstring OutString(WideCharNumber, 0);

	// 실제 변환을 수행
	MultiByteToWideChar(CP_UTF8, 0,
						InString.c_str(), -1, OutString.data(), WideCharNumber);

	// null 문자 제거
	OutString.resize(WideCharNumber - 1);

	return OutString;
}

/**
 * @brief CP949 (ANSI) 타입의 문자열을 UTF-8 타입으로 변환하는 함수
 * CP949 To UTF-8 직변환 함수가 제공되지 않아 UTF-16을 브릿지로 사용한다
 * @param InANSIString ANSI 문자열
 * @return UTF-8 문자열
 */
static string ConvertCP949ToUTF8(const char* InANSIString)
{
	if (InANSIString == nullptr || InANSIString[0] == '\0')
	{
		return "";
	}

	// CP949 -> UTF-8 변환
	int WideCharacterSize = MultiByteToWideChar(CP_ACP, 0, InANSIString, -1, nullptr, 0);
	if (WideCharacterSize == 0)
	{
		return "";
	}

	wstring WideString(WideCharacterSize, 0);
	MultiByteToWideChar(CP_ACP, 0, InANSIString, -1, WideString.data(), WideCharacterSize);

	// UTF-16 -> UTF-8 변환
	int UTF8Size = WideCharToMultiByte(CP_UTF8, 0, WideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (UTF8Size == 0)
	{
		return "";
	}

	string UTF8String(UTF8Size, 0);
	WideCharToMultiByte(CP_UTF8, 0, WideString.c_str(), -1, UTF8String.data(), UTF8Size, nullptr, nullptr);

	if (!UTF8String.empty() && UTF8String.back() == '\0')
	{
		UTF8String.pop_back();
	}

	return UTF8String;
}
