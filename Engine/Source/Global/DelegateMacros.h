#pragma once


////////////////////////////////////////////////////////////////
/**
 * @file DelegateMacros.h
 * @brief 델리게이트 선언용 매크로 정의 헤더
 *
 * @details
 * 현재는 모든 델리게이트가 단일 템플릿 클래스 `TDelegate`로 처리되지만,  
 * 매크로를 기능별로 분리하여 **의도를 명확히 표현**하고  
 * **향후 확장(TMulticastDelegate, TDynamicDelegate 등)** 시  
 * 매크로 정의만 수정하면 전체 코드를 손대지 않고 대응할 수 있도록 설계됨.
 *
 * 예:
 * @code
 * DECLARE_DELEGATE(FOnDamageTaken, int);
 * DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthChanged, int);
 * @endcode
 */
////////////////////////////////////////////////////////////////


/**
 * @def DECLARE_DELEGATE
 * @brief 일반 단일 델리게이트를 선언합니다.
 *
 * @details
 * 일반적인 함수나 람다를 바인딩할 때 사용하는 가장 기본적인 델리게이트입니다.  
 * UObject 수명 관리나 멀티캐스트 기능은 포함되지 않습니다.
 *
 * @param DelegateName 델리게이트 타입 이름
 * @param ... 델리게이트가 전달받을 매개변수 타입 목록
 *
 * @note
 * 향후 `TDelegate`가 `TSingleCastDelegate` 또는 `TBaseDelegate`로 교체될 수 있습니다.
 *
 * @see DECLARE_MULTICAST_DELEGATE
 * @see DECLARE_DYNAMIC_DELEGATE
 */
#define DECLARE_DELEGATE(DelegateName, ...) \
    using DelegateName = TDelegate<__VA_ARGS__>

/**
 * @def DECLARE_MULTICAST_DELEGATE
 * @brief 여러 개의 핸들러를 바인딩할 수 있는 멀티캐스트 델리게이트를 선언합니다.
 *
 * @details
 * 동일한 이벤트를 여러 객체가 수신해야 할 때 사용합니다.  
 * 현재는 `TDelegate`를 그대로 사용하지만,  
 * 향후 `TMulticastDelegate` 등으로 확장될 예정입니다.
 *
 * @param DelegateName 델리게이트 타입 이름
 * @param ... 델리게이트가 전달받을 매개변수 타입 목록
 *
 * @see DECLARE_DELEGATE
 * @see DECLARE_DYNAMIC_MULTICAST_DELEGATE
 */
#define DECLARE_MULTICAST_DELEGATE(DelegateName, ...) \
    using DelegateName = TDelegate<__VA_ARGS__>

/**
 * @def DECLARE_DYNAMIC_DELEGATE
 * @brief UObject 기반 클래스 멤버 함수를 안전하게 바인딩할 수 있는 델리게이트를 선언합니다.
 *
 * @details
 * 내부적으로 `TWeakObjectPtr`을 사용하여 UObject의 생명주기를 추적합니다.  
 * UObject가 파괴되면 자동으로 무효화되어 안전하게 처리됩니다.
 *
 * @param DelegateName 델리게이트 타입 이름
 * @param ... 델리게이트가 전달받을 매개변수 타입 목록
 *
 * @note
 * 현재는 `TDelegate`와 동일하게 동작하지만,  
 * 추후 리플렉션이나 GC 연동이 필요한 `TDynamicDelegate`로 교체될 수 있습니다.
 *
 * @see DECLARE_DYNAMIC_MULTICAST_DELEGATE
 */
#define DECLARE_DYNAMIC_DELEGATE(DelegateName, ...) \
    using DelegateName = TDelegate<__VA_ARGS__>

/**
 * @def DECLARE_DYNAMIC_MULTICAST_DELEGATE
 * @brief UObject-safe 멀티캐스트 델리게이트를 선언합니다.
 *
 * @details
 * 여러 UObject 기반 객체의 멤버 함수를 바인딩할 수 있으며,  
 * UObject가 파괴되면 자동으로 무효화되어 크래시를 방지합니다.  
 * 현재는 `TDelegate`를 재사용하고 있지만,  
 * 향후 `TDynamicMulticastDelegate`로 확장될 수 있습니다.
 *
 * @param DelegateName 델리게이트 타입 이름
 * @param ... 델리게이트가 전달받을 매개변수 타입 목록
 *
 * @see DECLARE_MULTICAST_DELEGATE
 * @see DECLARE_DYNAMIC_DELEGATE
 */
#define DECLARE_DYNAMIC_MULTICAST_DELEGATE(DelegateName, ...) \
    using DelegateName = TDelegate<__VA_ARGS__>