#pragma once
#include "Component/Collision/Public/SphereComponent.h"
#include "Component/Collision/Public/BoxComponent.h"
#include "Component/Collision/Public/CapsuleComponent.h"

/**
 * @file CollisionUtil.h
 * @brief Narrow Phase 충돌 검사 유틸리티 함수들
 * @note 모든 충돌 검사는 월드 좌표계 기준으로 수행됩니다
 * @note Broad Phase는 옥트리에서 처리하고, 이 함수들은 정밀 검사용입니다
 */

namespace CollisionUtil
{
    /*-----------------------------------------------------------------------------
        Helper Functions (내부 사용)
    -----------------------------------------------------------------------------*/

    /**
     * @brief 세 값 중 최대값을 반환합니다
     * @param A 첫 번째 값
     * @param B 두 번째 값
     * @param C 세 번째 값
     * @return 최대값
     */
    inline float Max3(float A, float B, float C)
    {
        return std::max(std::max(A, B), C);
    }
    // ============================================================================
    // Capsule Collision Helpers
    // ============================================================================

  /**
   * @brief 두 선분(Line Segment) 간의 최단 거리를 계산합니다.
   * @param SegmentAStart 첫 번째 선분의 시작점
   * @param SegmentAEnd 첫 번째 선분의 끝점
   * @param SegmentBStart 두 번째 선분의 시작점
   * @param SegmentBEnd 두 번째 선분의 끝점
   * @param OutClosestPointA 첫 번째 선분 위의 최근접점 (출력)
   * @param OutClosestPointB 두 번째 선분 위의 최근접점 (출력)
   * @return 두 선분 간의 최단 거리
   */
    inline float ClosestPointsBetweenSegments(
        const FVector& SegmentAStart, const FVector& SegmentAEnd,
        const FVector& SegmentBStart, const FVector& SegmentBEnd,
        FVector& OutClosestPointA, FVector& OutClosestPointB)
    {
        FVector DirectionA = SegmentAEnd - SegmentAStart;
        FVector DirectionB = SegmentBEnd - SegmentBStart;
        FVector OffsetVector = SegmentAStart - SegmentBStart;

        float LengthA = DirectionA.Dot(DirectionA);  // |d0|^2
        float LengthB = DirectionB.Dot(DirectionB);  // |d1|^2
        float ProjectionBA = DirectionB.Dot(OffsetVector);  // d1 · (P0 - Q0)

        // 두 선분이 모두 점인 경우
        if (LengthA <= 0.00001f && LengthB <= 0.00001f)
        {
            OutClosestPointA = SegmentAStart;
            OutClosestPointB = SegmentBStart;
            return FVector::Dist(SegmentAStart, SegmentBStart);
        }

        // 선분 A가 점인 경우
        if (LengthA <= 0.00001f)
        {
            float ParameterB = Clamp(-ProjectionBA / LengthB, 0.0f, 1.0f);
            OutClosestPointA = SegmentAStart;
            OutClosestPointB = SegmentBStart + ParameterB * DirectionB;
            return FVector::Dist(OutClosestPointA, OutClosestPointB);
        }

        float ProjectionAB = DirectionA.Dot(OffsetVector);  // d0 · (P0 - Q0)

        // 선분 B가 점인 경우
        if (LengthB <= 0.00001f)
        {
            float ParameterA = Clamp(ProjectionAB / LengthA, 0.0f, 1.0f);
            OutClosestPointA = SegmentAStart + ParameterA * DirectionA;
            OutClosestPointB = SegmentBStart;
            return FVector::Dist(OutClosestPointA, OutClosestPointB);
        }

        // 일반적인 경우: 두 선분이 모두 유효함
        float ProjectionABDot = DirectionA.Dot(DirectionB);  // d0 · d1
        float Denominator = LengthA * LengthB - ProjectionABDot * ProjectionABDot;

        float ParameterA = 0.0f;
        if (Denominator != 0.0f)
        {
            ParameterA = Clamp((ProjectionABDot * ProjectionBA - ProjectionAB * LengthB) / Denominator, 0.0f, 1.0f);
        }

        float ParameterB = (ProjectionABDot * ParameterA + ProjectionBA) / LengthB;

        // ParameterB가 [0, 1] 범위를 벗어나면 재계산
        if (ParameterB < 0.0f)
        {
            ParameterB = 0.0f;
            ParameterA = Clamp(-ProjectionAB / LengthA, 0.0f, 1.0f);
        }
        else if (ParameterB > 1.0f)
        {
            ParameterB = 1.0f;
            ParameterA = Clamp((ProjectionABDot - ProjectionAB) / LengthA, 0.0f, 1.0f);
        }

        OutClosestPointA = SegmentAStart + ParameterA * DirectionA;
        OutClosestPointB = SegmentBStart + ParameterB * DirectionB;

        return FVector::Dist(OutClosestPointA, OutClosestPointB);
    }

    /**
     * @brief 점과 OBB(Oriented Bounding Box)의 최근접점을 계산합니다.
     * @param OBB OBB 정보
     * @param Point 임의의 점
     * @return OBB 표면 위의 최근접점
     */
    inline FVector ClosestPointOnOBB(const FOBB& OBB, const FVector& Point)
    {
        // OBB의 로컬 좌표계로 변환
        FVector LocalPoint = Point - OBB.Center;

        // OBB의 3개 축 추출
        FVector AxisX(OBB.ScaleRotation.Data[0][0], OBB.ScaleRotation.Data[1][0], OBB.ScaleRotation.Data[2][0]);
        FVector AxisY(OBB.ScaleRotation.Data[0][1], OBB.ScaleRotation.Data[1][1], OBB.ScaleRotation.Data[2][1]);
        FVector AxisZ(OBB.ScaleRotation.Data[0][2], OBB.ScaleRotation.Data[1][2], OBB.ScaleRotation.Data[2][2]);

        // 각 축에 투영하여 로컬 좌표 계산
        float ProjectionX = LocalPoint.Dot(AxisX);
        float ProjectionY = LocalPoint.Dot(AxisY);
        float ProjectionZ = LocalPoint.Dot(AxisZ);

        // OBB의 범위로 클램핑
        ProjectionX = Clamp(ProjectionX, -OBB.Extents.X, OBB.Extents.X);
        ProjectionY = Clamp(ProjectionY, -OBB.Extents.Y, OBB.Extents.Y);
        ProjectionZ = Clamp(ProjectionZ, -OBB.Extents.Z, OBB.Extents.Z);

        // 월드 좌표로 다시 변환
        FVector ClosestPoint = OBB.Center;
        ClosestPoint += ProjectionX * AxisX;
        ClosestPoint += ProjectionY * AxisY;
        ClosestPoint += ProjectionZ * AxisZ;

        return ClosestPoint;
    }

    /**
     * @brief 선분과 OBB 간의 최단 거리를 계산합니다.
     * @param SegmentStart 선분의 시작점
     * @param SegmentEnd 선분의 끝점
     * @param OBB OBB 정보
     * @return 선분과 OBB 표면 간의 최단 거리
     * TODO(SDM) - 이 또한 어느 정도 근사치이다. 정확한 계산에 비용이 너무 많이 들면 이것으로 바꿀 것.
     */
    /*
    inline float DistanceSegmentToOBB(const FVector& SegmentStart, const FVector& SegmentEnd, const FOBB& OBB) 
    {
        // 선분의 양 끝점에서 OBB까지의 최근접점을 구함
        FVector ClosestPointStart = ClosestPointOnOBB(OBB, SegmentStart);
        FVector ClosestPointEnd = ClosestPointOnOBB(OBB, SegmentEnd);

        float DistanceStart = FVector::Dist(SegmentStart, ClosestPointStart);
        float DistanceEnd = FVector::Dist(SegmentEnd, ClosestPointEnd);

        // 선분 위의 여러 샘플 포인트를 테스트 (정확도 향상)
        float MinDistance = min(DistanceStart, DistanceEnd);

		constexpr int32 SampleCount = 8; // 정확도를 높이고 싶다면 더 늘릴 수 있음
        for (int32 SampleIndex = 1; SampleIndex < SampleCount; ++SampleIndex)
        {
            float InterpolationFactor = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
            FVector SamplePoint = SegmentStart + InterpolationFactor * (SegmentEnd - SegmentStart);
            FVector ClosestPoint = ClosestPointOnOBB(OBB, SamplePoint);
            float Distance = FVector::Dist(SamplePoint, ClosestPoint);
            MinDistance = min(MinDistance, Distance);
        }

        return MinDistance;
    }
    */

    // ============================================================================
  // 선분-OBB 정확한 거리 계산 (헬퍼 함수들)
  // ============================================================================

  /**
   * @brief 점과 평면 사이의 부호있는 거리를 계산합니다.
   * @param Point 점
   * @param PlaneOrigin 평면 위의 한 점
   * @param PlaneNormal 평면의 법선 벡터 (정규화되어야 함)
   * @return 부호있는 거리 (양수: 법선 방향, 음수: 반대 방향)
   */
    inline float DistancePointToPlane(const FVector& Point, const FVector& PlaneOrigin, const FVector& PlaneNormal)
    {
        return (Point - PlaneOrigin).Dot(PlaneNormal);
    }

    /**
     * @brief 점을 평면에 투영한 점을 반환합니다.
     * @param Point 투영할 점
     * @param PlaneOrigin 평면 위의 한 점
     * @param PlaneNormal 평면의 법선 벡터 (정규화되어야 함)
     * @return 평면에 투영된 점
     */
    inline FVector ProjectPointOnPlane(const FVector& Point, const FVector& PlaneOrigin, const FVector& PlaneNormal)
    {
        float Distance = DistancePointToPlane(Point, PlaneOrigin, PlaneNormal);
        return Point - PlaneNormal * Distance;
    }

    /**
     * @brief 점이 사각형(평면 위의 4개 꼭짓점) 내부에 있는지 확인합니다.
     * @param Point 평면에 투영된 점
     * @param RectCenter 사각형 중심
     * @param AxisU 사각형의 첫 번째 축 (정규화됨)
     * @param AxisV 사각형의 두 번째 축 (정규화됨)
     * @param HalfExtentU 첫 번째 축 방향 반 크기
     * @param HalfExtentV 두 번째 축 방향 반 크기
     * @return 사각형 내부면 true
     */
    inline bool IsPointInsideRectangle(
        const FVector& Point,
        const FVector& RectCenter,
        const FVector& AxisU,
        const FVector& AxisV,
        float HalfExtentU,
        float HalfExtentV)
    {
        FVector LocalPoint = Point - RectCenter;
        float ProjectionU = LocalPoint.Dot(AxisU);
        float ProjectionV = LocalPoint.Dot(AxisV);

        return (ProjectionU >= -HalfExtentU && ProjectionU <= HalfExtentU &&
            ProjectionV >= -HalfExtentV && ProjectionV <= HalfExtentV);
    }

    /**
     * @brief 점을 사각형(평면 위의 4개 꼭짓점) 내부로 클램핑합니다.
     * @param Point 평면에 투영된 점
     * @param RectCenter 사각형 중심
     * @param AxisU 사각형의 첫 번째 축 (정규화됨)
     * @param AxisV 사각형의 두 번째 축 (정규화됨)
     * @param HalfExtentU 첫 번째 축 방향 반 크기
     * @param HalfExtentV 두 번째 축 방향 반 크기
     * @return 사각형 경계 내의 최근접점
     */
    inline FVector ClampPointToRectangle(
        const FVector& Point,
        const FVector& RectCenter,
        const FVector& AxisU,
        const FVector& AxisV,
        float HalfExtentU,
        float HalfExtentV)
    {
        FVector LocalPoint = Point - RectCenter;
        float ProjectionU = Clamp(LocalPoint.Dot(AxisU), -HalfExtentU, HalfExtentU);
        float ProjectionV = Clamp(LocalPoint.Dot(AxisV), -HalfExtentV, HalfExtentV);

        return RectCenter + AxisU * ProjectionU + AxisV * ProjectionV;
    }

    /**
     * @brief 선분과 평면(무한) 사이의 최단 거리를 계산합니다.
     * @param SegmentStart 선분의 시작점
     * @param SegmentEnd 선분의 끝점
     * @param PlaneOrigin 평면 위의 한 점
     * @param PlaneNormal 평면의 법선 벡터 (정규화되어야 함)
     * @param OutClosestOnSegment 선분 위의 최근접점 (출력)
     * @param OutClosestOnPlane 평면 위의 최근접점 (출력)
     * @return 최단 거리
     */
    inline float DistanceSegmentToPlane(
        const FVector& SegmentStart,
        const FVector& SegmentEnd,
        const FVector& PlaneOrigin,
        const FVector& PlaneNormal,
        FVector& OutClosestOnSegment,
        FVector& OutClosestOnPlane)
    {
        float DistStart = DistancePointToPlane(SegmentStart, PlaneOrigin, PlaneNormal);
        float DistEnd = DistancePointToPlane(SegmentEnd, PlaneOrigin, PlaneNormal);

        // 선분이 평면을 관통하는 경우
        if ((DistStart > 0.0f && DistEnd < 0.0f) || (DistStart < 0.0f && DistEnd > 0.0f))
        {
            // 교차점 계산
            float TotalDist = DistStart - DistEnd;
            if (TotalDist != 0.0f)
            {
                float T = DistStart / TotalDist;
                OutClosestOnSegment = SegmentStart + T * (SegmentEnd - SegmentStart);
                OutClosestOnPlane = OutClosestOnSegment;
                return 0.0f;
            }
        }

        // 선분이 평면을 관통하지 않는 경우
        if (DistStart * DistStart < DistEnd * DistEnd)
        {
            OutClosestOnSegment = SegmentStart;
            OutClosestOnPlane = ProjectPointOnPlane(SegmentStart, PlaneOrigin, PlaneNormal);
            return (DistStart > 0.0f) ? DistStart : -DistStart;
        }
        else
        {
            OutClosestOnSegment = SegmentEnd;
            OutClosestOnPlane = ProjectPointOnPlane(SegmentEnd, PlaneOrigin, PlaneNormal);
            return (DistEnd > 0.0f) ? DistEnd : -DistEnd;
        }
    }

    /**
     * @brief 선분과 사각형(OBB 면) 사이의 최단 거리를 계산합니다.
     * @param SegmentStart 선분의 시작점
     * @param SegmentEnd 선분의 끝점
     * @param RectCenter 사각형 중심
     * @param AxisU 사각형의 첫 번째 축 (정규화됨)
     * @param AxisV 사각형의 두 번째 축 (정규화됨)
     * @param PlaneNormal 평면의 법선 벡터 (정규화됨)
     * @param HalfExtentU 첫 번째 축 방향 반 크기
     * @param HalfExtentV 두 번째 축 방향 반 크기
     * @return 최단 거리
     */
    inline float DistanceSegmentToRectangle(
        const FVector& SegmentStart,
        const FVector& SegmentEnd,
        const FVector& RectCenter,
        const FVector& AxisU,
        const FVector& AxisV,
        const FVector& PlaneNormal,
        float HalfExtentU,
        float HalfExtentV)
    {
        // 1. 선분과 평면의 최근접점 계산
        FVector ClosestOnSegment, ClosestOnPlane;
        float DistToPlane = DistanceSegmentToPlane(SegmentStart, SegmentEnd, RectCenter, PlaneNormal, ClosestOnSegment,
            ClosestOnPlane);

        // 2. 평면 위의 최근접점이 사각형 내부인지 확인
        if (IsPointInsideRectangle(ClosestOnPlane, RectCenter, AxisU, AxisV, HalfExtentU, HalfExtentV))
        {
            return DistToPlane;
        }

        // 3. 사각형 밖이면, 사각형 경계로 클램핑한 후 거리 재계산
        FVector ClampedPoint = ClampPointToRectangle(ClosestOnPlane, RectCenter, AxisU, AxisV, HalfExtentU, HalfExtentV);

        // 클램핑된 점에서 선분까지의 최단 거리
        FVector SegmentDir = SegmentEnd - SegmentStart;
        FVector ToPoint = ClampedPoint - SegmentStart;
        float SegmentLengthSq = SegmentDir.Dot(SegmentDir);

        if (SegmentLengthSq < 0.00001f)
        {
            return FVector::Dist(SegmentStart, ClampedPoint);
        }

        float T = Clamp(ToPoint.Dot(SegmentDir) / SegmentLengthSq, 0.0f, 1.0f);
        FVector ClosestOnSeg = SegmentStart + T * SegmentDir;

        return FVector::Dist(ClosestOnSeg, ClampedPoint);
    }

    /**
  * @brief 선분과 OBB 간의 정확한 최단 거리를 계산합니다 (내부 구현).
  * @param SegmentStart 선분의 시작점
  * @param SegmentEnd 선분의 끝점
  * @param OBB OBB 정보
  * @return 선분과 OBB 표면 간의 최단 거리
  */
    inline float DistanceSegmentToOBB_Exact(const FVector& SegmentStart, const FVector& SegmentEnd, const FOBB& OBB)
    {
        // OBB의 3개 축 추출 (열 우선)
        FVector AxisX(OBB.ScaleRotation.Data[0][0], OBB.ScaleRotation.Data[1][0], OBB.ScaleRotation.Data[2][0]);
        FVector AxisY(OBB.ScaleRotation.Data[0][1], OBB.ScaleRotation.Data[1][1], OBB.ScaleRotation.Data[2][1]);
        FVector AxisZ(OBB.ScaleRotation.Data[0][2], OBB.ScaleRotation.Data[1][2], OBB.ScaleRotation.Data[2][2]);

        AxisX.Normalize();
        AxisY.Normalize();
        AxisZ.Normalize();

        float MinDistance = FLT_MAX;

        // ========== 1. 선분 끝점과 OBB의 거리 ==========
        FVector ClosestStart = ClosestPointOnOBB(OBB, SegmentStart);
        FVector ClosestEnd = ClosestPointOnOBB(OBB, SegmentEnd);

        MinDistance = FVector::Dist(SegmentStart, ClosestStart);
        float DistEnd = FVector::Dist(SegmentEnd, ClosestEnd);
        if (DistEnd < MinDistance)
        {
            MinDistance = DistEnd;
        }

        // ========== 2. 선분과 OBB 6개 면의 거리 ==========

        // 면 구조: {중심, AxisU, AxisV, 법선, HalfExtentU, HalfExtentV}
        struct Face
        {
            FVector Center;
            FVector AxisU;
            FVector AxisV;
            FVector Normal;
            float HalfExtentU;
            float HalfExtentV;
        };

        Face Faces[6] =
        {
            // +X 면
            { OBB.Center + AxisX * OBB.Extents.X, AxisY, AxisZ, AxisX, OBB.Extents.Y, OBB.Extents.Z },
            // -X 면
            { OBB.Center - AxisX * OBB.Extents.X, AxisY, AxisZ, -AxisX, OBB.Extents.Y, OBB.Extents.Z },
            // +Y 면
            { OBB.Center + AxisY * OBB.Extents.Y, AxisX, AxisZ, AxisY, OBB.Extents.X, OBB.Extents.Z },
            // -Y 면
            { OBB.Center - AxisY * OBB.Extents.Y, AxisX, AxisZ, -AxisY, OBB.Extents.X, OBB.Extents.Z },
            // +Z 면
            { OBB.Center + AxisZ * OBB.Extents.Z, AxisX, AxisY, AxisZ, OBB.Extents.X, OBB.Extents.Y },
            // -Z 면
            { OBB.Center - AxisZ * OBB.Extents.Z, AxisX, AxisY, -AxisZ, OBB.Extents.X, OBB.Extents.Y }
        };

        for (int32 FaceIndex = 0; FaceIndex < 6; ++FaceIndex)
        {
            const Face& F = Faces[FaceIndex];
            float Distance = DistanceSegmentToRectangle(
                SegmentStart, SegmentEnd,
                F.Center, F.AxisU, F.AxisV, F.Normal,
                F.HalfExtentU, F.HalfExtentV
            );

            if (Distance < MinDistance)
            {
                MinDistance = Distance;
            }

            // 조기 종료: 이미 충분히 가까우면 더 이상 계산 불필요
            if (MinDistance < 0.001f)
            {
                return MinDistance;
            }
        }

        // ========== 3. 선분과 OBB 12개 모서리의 거리 ==========

        // OBB의 8개 꼭짓점
        FVector Vertices[8];
        int32 VertexIndex = 0;
        for (int32 SignX = -1; SignX <= 1; SignX += 2)
        {
            for (int32 SignY = -1; SignY <= 1; SignY += 2)
            {
                for (int32 SignZ = -1; SignZ <= 1; SignZ += 2)
                {
                    Vertices[VertexIndex++] = OBB.Center +
                        AxisX * (SignX * OBB.Extents.X) +
                        AxisY * (SignY * OBB.Extents.Y) +
                        AxisZ * (SignZ * OBB.Extents.Z);
                }
            }
        }

        // 12개 모서리 (각 모서리는 두 꼭짓점의 인덱스)
        int32 Edges[12][2] =
        {
            // X축 방향 모서리 (4개)
            {0, 1}, {2, 3}, {4, 5}, {6, 7},
            // Y축 방향 모서리 (4개)
            {0, 2}, {1, 3}, {4, 6}, {5, 7},
            // Z축 방향 모서리 (4개)
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        for (int32 EdgeIndex = 0; EdgeIndex < 12; ++EdgeIndex)
        {
            FVector EdgeStart = Vertices[Edges[EdgeIndex][0]];
            FVector EdgeEnd = Vertices[Edges[EdgeIndex][1]];

            FVector ClosestOnSegment, ClosestOnEdge;
            float Distance = ClosestPointsBetweenSegments(
                SegmentStart, SegmentEnd,
                EdgeStart, EdgeEnd,
                ClosestOnSegment, ClosestOnEdge
            );

            if (Distance < MinDistance)
            {
                MinDistance = Distance;
            }

            // 조기 종료
            if (MinDistance < 0.001f)
            {
                return MinDistance;
            }
        }

        return MinDistance;
    }

    /**
     * @brief 선분과 OBB 간의 최단 거리를 계산합니다 (단계적 검사로 최적화됨).
     *
     * 최적화 전략:
     * 1단계 (빠른 체크): 선분 끝점 2개만 OBB와 거리 계산
     *   → 충분히 멀면 (임계값 초과) 조기 반환
     *   → 가까우면 2단계로 진행
     *
     * 2단계 (정밀 체크): 6개 면 + 12개 모서리까지 모두 계산
     *   → 정확한 최단 거리 반환
     *
     * @param SegmentStart 선분의 시작점
     * @param SegmentEnd 선분의 끝점
     * @param OBB OBB 정보
     * @return 선분과 OBB 표면 간의 최단 거리
     */
    inline float DistanceSegmentToOBB(const FVector& SegmentStart, const FVector& SegmentEnd, const FOBB& OBB)
    {
        // ========== 1단계: 빠른 체크 (선분 끝점만) ==========

        FVector ClosestStart = ClosestPointOnOBB(OBB, SegmentStart);
        FVector ClosestEnd = ClosestPointOnOBB(OBB, SegmentEnd);

        float DistStart = FVector::Dist(SegmentStart, ClosestStart);
        float DistEnd = FVector::Dist(SegmentEnd, ClosestEnd);
        float QuickDistance = (DistStart < DistEnd) ? DistStart : DistEnd;

        // 임계값 계산: 선분 길이의 50% (조정 가능)
        FVector SegmentVector = SegmentEnd - SegmentStart;
        float SegmentLength = SegmentVector.Length();
        float Threshold = SegmentLength * 0.5f;

        // 임계값 최소/최대 제한
        // - 최소: 10 units (너무 작은 임계값 방지)
        // - 최대: 100 units (너무 큰 임계값 방지)
        Threshold = Clamp(Threshold, 10.0f, 100.0f);

        // 조기 반환: 끝점들이 충분히 멀리 떨어져 있음
        if (QuickDistance > Threshold)
        {
            return QuickDistance;
        }

        // ========== 2단계: 정밀 체크 (전체 계산) ==========

        return DistanceSegmentToOBB_Exact(SegmentStart, SegmentEnd, OBB);
    }

    /*-----------------------------------------------------------------------------
          Sphere vs Sphere Collision
      -----------------------------------------------------------------------------*/

      /**
       * @brief 두 Sphere 컴포넌트가 겹치는지 정밀 검사합니다
       * @param SphereA 첫 번째 Sphere 컴포넌트
       * @param SphereB 두 번째 Sphere 컴포넌트
       * @return 겹치면 true, 아니면 false
       *
       * @details 알고리즘:
       * 1. 두 Sphere의 월드 위치(중심점)를 가져옴
       * 2. 월드 스케일을 고려하여 실제 반지름 계산
       * 3. 두 중심점 사이 거리의 제곱 계산 (sqrt 비용 절약)
       * 4. 반지름 합의 제곱과 비교
       * 5. 거리² <= (반지름A + 반지름B)² 이면 충돌
       */
    inline bool TestOverlap(const USphereComponent* SphereA, const USphereComponent* SphereB)
    {
        if (!SphereA || !SphereB)
        {
            return false;
        }

        // 월드 위치 가져오기
        FVector CenterA = SphereA->GetWorldLocation();
        FVector CenterB = SphereB->GetWorldLocation();

        // 월드 스케일 가져오기
        FVector ScaleA = SphereA->GetWorldScale3D();
        FVector ScaleB = SphereB->GetWorldScale3D();

        // Sphere는 균등 스케일을 가정 (최대값 사용)
        float MaxScaleA = Max3(ScaleA.X, ScaleA.Y, ScaleA.Z);
        float MaxScaleB = Max3(ScaleB.X, ScaleB.Y, ScaleB.Z);

        // 실제 반지름
        float RadiusA = SphereA->GetSphereRadius() * MaxScaleA;
        float RadiusB = SphereB->GetSphereRadius() * MaxScaleB;

        // 두 중심점 사이 벡터
        FVector Delta = CenterB - CenterA;

        // 거리의 제곱
        float DistanceSquared = Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z;

        // 반지름 합의 제곱
        float RadiusSum = RadiusA + RadiusB;
        float RadiusSumSquared = RadiusSum * RadiusSum;

        return DistanceSquared <= RadiusSumSquared;
    }

    /*-----------------------------------------------------------------------------
        Box vs Box Collision (OBB - FOBB 활용)
    -----------------------------------------------------------------------------*/

    /**
     * @brief 두 Box 컴포넌트가 겹치는지 정밀 검사합니다 (회전 고려, OBB)
     * @param BoxA 첫 번째 Box 컴포넌트
     * @param BoxB 두 번째 Box 컴포넌트
     * @return 겹치면 true, 아니면 false
     *
     * @details 알고리즘:
     * 1. 각 BoxComponent에서 FOBB 생성
     * 2. FOBB::Intersects() 호출 (SAT 15축 테스트)
     *
     * @note FOBB가 이미 완벽한 SAT 구현을 제공하므로 재사용
     */
    inline bool TestOverlap(const UBoxComponent* BoxA, const UBoxComponent* BoxB)
    {
        if (!BoxA || !BoxB)
        {
            return false;
        }

        // BoxA FOBB 생성
        FVector CenterA = BoxA->GetWorldLocation();
        FVector ExtentA = BoxA->GetBoxExtent();
        const FMatrix& TransformA = BoxA->GetWorldTransformMatrix();

        FOBB OBBA(CenterA, ExtentA, TransformA);

        // BoxB FOBB 생성
        FVector CenterB = BoxB->GetWorldLocation();
        FVector ExtentB = BoxB->GetBoxExtent();
        const FMatrix& TransformB = BoxB->GetWorldTransformMatrix();

        FOBB OBBB(CenterB, ExtentB, TransformB);

        // FOBB의 SAT 충돌 검사 활용
        return OBBA.Intersects(OBBB);
    }

    /*-----------------------------------------------------------------------------
        Sphere vs Box Collision (OBB 고려)
    -----------------------------------------------------------------------------*/

    /**
     * @brief Sphere와 Box(OBB)가 겹치는지 정밀 검사합니다
     * @param Sphere Sphere 컴포넌트
     * @param Box Box 컴포넌트 (회전 고려)
     * @return 겹치면 true, 아니면 false
     *
     * @details 알고리즘:
     * 1. Sphere 중심을 Box의 로컬 좌표계로 변환
     * 2. 로컬 좌표계에서 가장 가까운 점 찾기 (Clamp)
     * 3. 다시 월드 좌표계로 변환
     * 4. 거리 비교
     */
    inline bool TestOverlap(const USphereComponent* Sphere, const UBoxComponent* Box)
    {
        if (!Sphere || !Box)
        {
            return false;
        }

        // Sphere 정보
        FVector SphereCenter = Sphere->GetWorldLocation();
        FVector SphereScale = Sphere->GetWorldScale3D();
        float MaxSphereScale = Max3(SphereScale.X, SphereScale.Y, SphereScale.Z);
        float SphereRadius = Sphere->GetSphereRadius() * MaxSphereScale;

        // Box 정보
        FVector BoxCenter = Box->GetWorldLocation();
        FVector BoxScale = Box->GetWorldScale3D();
        FVector BoxExtent = Box->GetBoxExtent();

        FVector BoxHalfSize = FVector(
            BoxExtent.X * BoxScale.X,
            BoxExtent.Y * BoxScale.Y,
            BoxExtent.Z * BoxScale.Z
        );

        // Box의 월드 변환 행렬
        const FMatrix& BoxTransform = Box->GetWorldTransformMatrix();

        // Box 로컬 축 추출 (행렬의 열)
        FVector BoxAxisX = FVector(BoxTransform.Data[0][0], BoxTransform.Data[1][0], BoxTransform.Data[2][0]);
        FVector BoxAxisY = FVector(BoxTransform.Data[0][1], BoxTransform.Data[1][1], BoxTransform.Data[2][1]);
        FVector BoxAxisZ = FVector(BoxTransform.Data[0][2], BoxTransform.Data[1][2], BoxTransform.Data[2][2]);

        // Sphere 중심을 Box 중심 기준 상대 위치로
        FVector RelativePos = SphereCenter - BoxCenter;

        // Sphere 중심을 Box 로컬 좌표계로 변환 (내적으로 투영)
        float LocalX = RelativePos.X * BoxAxisX.X + RelativePos.Y * BoxAxisX.Y + RelativePos.Z * BoxAxisX.Z;
        float LocalY = RelativePos.X * BoxAxisY.X + RelativePos.Y * BoxAxisY.Y + RelativePos.Z * BoxAxisY.Z;
        float LocalZ = RelativePos.X * BoxAxisZ.X + RelativePos.Y * BoxAxisZ.Y + RelativePos.Z * BoxAxisZ.Z;

        // Box 로컬 공간에서 가장 가까운 점 (Clamp)
        float ClosestX = Clamp(LocalX, -BoxHalfSize.X, BoxHalfSize.X);
        float ClosestY = Clamp(LocalY, -BoxHalfSize.Y, BoxHalfSize.Y);
        float ClosestZ = Clamp(LocalZ, -BoxHalfSize.Z, BoxHalfSize.Z);

        // 가장 가까운 점을 다시 월드 좌표로 변환
        FVector ClosestPointWorld = BoxCenter +
            BoxAxisX * ClosestX +
            BoxAxisY * ClosestY +
            BoxAxisZ * ClosestZ;

        // Sphere 중심과 가장 가까운 점 사이의 거리
        FVector Delta = ClosestPointWorld - SphereCenter;
        float DistanceSquared = Delta.X * Delta.X + Delta.Y * Delta.Y + Delta.Z * Delta.Z;

        return DistanceSquared <= (SphereRadius * SphereRadius);
    }

    inline bool TestOverlap(const UBoxComponent* Box, const USphereComponent* Sphere)
    {
        return TestOverlap(Sphere, Box);
    }

    // ============================================================================
   // Capsule vs Capsule
   // ============================================================================

    inline bool TestOverlap(const UCapsuleComponent* CapsuleA, const UCapsuleComponent* CapsuleB)
    {
        if (!CapsuleA || !CapsuleB)
        {
            return false;
        }

        // 캡슐의 중심축(선분) 계산
        FVector CenterA = CapsuleA->GetWorldLocation();
        float HalfHeightA = CapsuleA->GetCapsuleHalfHeight();
        FVector UpDirectionA = CapsuleA->GetUpVector();

        FVector SegmentAStart = CenterA - UpDirectionA * HalfHeightA;
        FVector SegmentAEnd = CenterA + UpDirectionA * HalfHeightA;

        FVector CenterB = CapsuleB->GetWorldLocation();
        float HalfHeightB = CapsuleB->GetCapsuleHalfHeight();
        FVector UpDirectionB = CapsuleB->GetUpVector();

        FVector SegmentBStart = CenterB - UpDirectionB * HalfHeightB;
        FVector SegmentBEnd = CenterB + UpDirectionB * HalfHeightB;

        // 두 선분 간의 최단 거리 계산
        FVector ClosestPointA, ClosestPointB;
        float Distance = ClosestPointsBetweenSegments(
            SegmentAStart, SegmentAEnd,
            SegmentBStart, SegmentBEnd,
            ClosestPointA, ClosestPointB
        );

        // 최단 거리가 두 반지름의 합보다 작으면 충돌
        float RadiusSum = CapsuleA->GetCapsuleRadius() + CapsuleB->GetCapsuleRadius();
        return Distance <= RadiusSum;
    }

    // ============================================================================
    // Capsule vs Sphere
    // ============================================================================

    inline bool TestOverlap(const UCapsuleComponent* Capsule, const USphereComponent* Sphere)
    {
        if (!Capsule || !Sphere)
        {
            return false;
        }

        // 캡슐의 중심축(선분) 계산
        FVector CapsuleCenter = Capsule->GetWorldLocation();
        float CapsuleHalfHeight = Capsule->GetCapsuleHalfHeight();
        FVector CapsuleUpDirection = Capsule->GetUpVector();

        FVector SegmentStart = CapsuleCenter - CapsuleUpDirection * CapsuleHalfHeight;
        FVector SegmentEnd = CapsuleCenter + CapsuleUpDirection * CapsuleHalfHeight;

        // 구의 중심에서 선분까지의 최단 거리 계산
        FVector LineDirection = SegmentEnd - SegmentStart;
        FVector PointToStart = Sphere->GetWorldLocation() - SegmentStart;

        float LineLengthSquared = LineDirection.Dot(LineDirection);
        float ProjectionParameter = 0.0f;

        if (LineLengthSquared > 0.00001f)
        {
            ProjectionParameter = Clamp(PointToStart.Dot(LineDirection) / LineLengthSquared, 0.0f, 1.0f);
        }

        FVector ClosestPointOnSegment = SegmentStart + ProjectionParameter * LineDirection;
        float Distance = FVector::Dist(Sphere->GetWorldLocation(), ClosestPointOnSegment);

        // 최단 거리가 두 반지름의 합보다 작으면 충돌
        float RadiusSum = Capsule->GetCapsuleRadius() + Sphere->GetSphereRadius();
        return Distance <= RadiusSum;
    }

    inline bool TestOverlap(const USphereComponent* Sphere, const UCapsuleComponent* Capsule)
    {
        return TestOverlap(Capsule, Sphere);
    }

    // ============================================================================
    // Capsule vs Box
    // ============================================================================

    inline bool TestOverlap(const UCapsuleComponent* Capsule, const UBoxComponent* Box)
    {
        if (!Capsule || !Box)
        {
            return false;
        }

        // 캡슐의 중심축(선분) 계산
        FVector CapsuleCenter = Capsule->GetWorldLocation();
        float CapsuleHalfHeight = Capsule->GetCapsuleHalfHeight();
        FVector CapsuleUpDirection = Capsule->GetUpVector();

        FVector SegmentStart = CapsuleCenter - CapsuleUpDirection * CapsuleHalfHeight;
        FVector SegmentEnd = CapsuleCenter + CapsuleUpDirection * CapsuleHalfHeight;

        // Box를 OBB로 변환
        FVector BoxCenter = Box->GetWorldLocation();
        FVector BoxExtent = Box->GetBoxExtent();
        const FMatrix& BoxTransform = Box->GetWorldTransformMatrix();
        FOBB OBB(BoxCenter, BoxExtent, BoxTransform);

        // 선분과 OBB 간의 최단 거리 계산
        float Distance = DistanceSegmentToOBB(SegmentStart, SegmentEnd, OBB);

        // 최단 거리가 캡슐 반지름보다 작으면 충돌
        return Distance <= Capsule->GetCapsuleRadius();
    }

    inline bool TestOverlap(const UBoxComponent* Box, const UCapsuleComponent* Capsule)
    {
        return TestOverlap(Capsule, Box);
    }

    /*-----------------------------------------------------------------------------
        Generic TestOverlap - 런타임 타입 판별
    -----------------------------------------------------------------------------*/

    /**
     * @brief 두 ShapeComponent의 타입을 판별하여 적절한 충돌 검사 수행
     */
    inline bool TestOverlap(const UShapeComponent* ShapeA, const UShapeComponent* ShapeB)
    {
        if (!ShapeA || !ShapeB)
        {
            return false;
        }

        UShapeComponent* MutableShapeA = const_cast<UShapeComponent*>(ShapeA);
        UShapeComponent* MutableShapeB = const_cast<UShapeComponent*>(ShapeB);

        const USphereComponent* SphereA = Cast<USphereComponent>(MutableShapeA);
        const USphereComponent* SphereB = Cast<USphereComponent>(MutableShapeB);
        const UBoxComponent* BoxA = Cast<UBoxComponent>(MutableShapeA);
        const UBoxComponent* BoxB = Cast<UBoxComponent>(MutableShapeB);
        const UCapsuleComponent* CapsuleA = Cast<UCapsuleComponent>(MutableShapeA);
        const UCapsuleComponent* CapsuleB = Cast<UCapsuleComponent>(MutableShapeB);

        // Sphere vs Sphere
        if (SphereA && SphereB) { return TestOverlap(SphereA, SphereB); }

        // Box vs Box
        if (BoxA && BoxB) { return TestOverlap(BoxA, BoxB); }

        // Sphere vs Box
        if (SphereA && BoxB) { return TestOverlap(SphereA, BoxB); }
        if (BoxA && SphereB) { return TestOverlap(BoxA, SphereB); }

        // Capsule vs Capsule
        if (CapsuleA && CapsuleB) { return TestOverlap(CapsuleA, CapsuleB); }

        // Capsule vs Sphere
        if (CapsuleA && SphereB) { return TestOverlap(CapsuleA, SphereB); }
        if (SphereA && CapsuleB) { return TestOverlap(SphereA, CapsuleB); }

        // Capsule vs Box
        if (CapsuleA && BoxB) { return TestOverlap(CapsuleA, BoxB); }
        if (BoxA && CapsuleB) { return TestOverlap(BoxA, CapsuleB); }

        return false;
    }

} // namespace CollisionUtil