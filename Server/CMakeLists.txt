# 프로젝트 이름을 'Server'로 설정
# 이 변수는 나중에 빌드될 실행 파일의 이름으로 사용
set(APP_NAME "Server")

# 소스 파일과 헤더 파일을 변수에 담아 관리
set(SOURCES
    src/main.cpp
    src/ClientSession.cpp
)

set(HEADERS
    include/ClientSession.hpp
)

# 소스 파일과 헤더 파일을 함께 타겟에 추가
add_executable(${APP_NAME} ${SOURCES} ${HEADERS})

# --- 미리 컴파일된 헤더(Precompiled Headers, PCH) 설정 ---
# PCH로 사용할 메인 헤더 파일을 지정
# 이 파일은 일반적으로 프로젝트의 include 디렉토리 내에 위치하며, 자주 사용되는 모든 헤더들을 포함
set(SERVER_PCH_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/Pch.hpp")
target_precompile_headers(${APP_NAME} PRIVATE ${SERVER_PCH_HEADER})

# 생성된 실행 파일(`APP_NAME` = Server)에 필요한 라이브러리들을 링크(연결)
# PRIVATE 키워드는 이 라이브러리들이 `APP_NAME`이라는 타겟 자체에만 필요하고,
# 이 타겟에 의존하는 다른 타겟들에게는 노출되지 않음을 의미
target_link_libraries(${APP_NAME} PRIVATE 
    ServerCore              # 서버 핵심 라이브러리
)

# 타겟에 인클루드 디렉토리 추가
target_include_directories(${APP_NAME} PRIVATE "include")
