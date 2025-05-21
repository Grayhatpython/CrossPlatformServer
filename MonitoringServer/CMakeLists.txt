# 프로젝트 이름을 'Server'로 설정
# 이 변수는 나중에 빌드될 실행 파일의 이름으로 사용
set(APP_NAME "MonitoringServer")

# --- ImGui 관련 설정 ---
# ImGui 라이브러리 소스 파일이 위치한 디렉토리를 IMGUI_DIR 변수로 설정
# ${CMAKE_SOURCE_DIR}는 현재 CMake 프로젝트의 최상위 소스 디렉토리 경로
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

# ImGui 라이브러리에 필요한 모든 소스 파일들을 IMGUI_SOURCES 변수에 명시적으로 나열
set(IMGUI_SOURCES 
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)
# --- ImGui 관련 설정 ---

# 서버의 주요 소스 파일들을 찾아서 SOURCES 변수에 저장
# GLOB는 편리하지만, 소스 파일이 추가되거나 삭제되었을 때 CMake를 수동으로 다시 실행해야 할 수 있다
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.hpp")

# 최종 실행 파일(`APP_NAME` 즉, "Server")을 생성
# 이 실행 파일을 만들기 위해 위에서 정의한 모든 소스 파일들(Server 소스, ImGui 소스)을 사용
add_executable(${APP_NAME} ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})

# --- 미리 컴파일된 헤더(Precompiled Headers, PCH) 설정 ---
# PCH로 사용할 메인 헤더 파일을 지정
# 이 파일은 일반적으로 프로젝트의 include 디렉토리 내에 위치하며, 자주 사용되는 모든 헤더들을 포함
set(MONITORINGSERVER_PCH_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/Pch.hpp")
target_precompile_headers(${APP_NAME} PRIVATE ${MONITORINGSERVER_PCH_HEADER})

# 생성된 실행 파일(`APP_NAME` = Server)에 필요한 라이브러리들을 링크(연결)
# PRIVATE 키워드는 이 라이브러리들이 `APP_NAME`이라는 타겟 자체에만 필요하고,
# 이 타겟에 의존하는 다른 타겟들에게는 노출되지 않음을 의미
target_link_libraries(${APP_NAME} PRIVATE 
    ServerCore              # 서버 핵심 라이브러리
    glfw                    # GLFW 라이브러리
    ${OPENGL_LIBRARIES}     # CMake의 find_package(OpenGL) 명령으로 찾아진 OpenGL 관련 라이브러리들
)

# Windows 운영체제인 경우에만 추가적인 링크 설정
# WIN32는 CMake가 Windows 플랫폼에서 실행될 때 자동으로 설정되는 내장 변수
if(WIN32)
    # Windows에서는 OpenGL 기능을 사용하기 위해 'opengl32' 라이브러리를 추가로 연결
    target_link_libraries(${APP_NAME} PRIVATE opengl32)
endif()

# 생성된 실행 파일(`APP_NAME` = Server)이 소스 코드에서 인클루드하는 헤더 파일들을 찾을 경로를 설정
# PRIVATE 키워드는 이 인클루드 디렉토리들이 `APP_NAME` 타겟의 컴파일에만 사용되고,
# 다른 타겟에게는 상속되지 않음을 의미
target_include_directories(${APP_NAME} PRIVATE 
    ${IMGUI_DIR}                                # ImGui 라이브러리의 헤더 파일 경로
    ${OPENGL_INCLUDE_DIR}                       # CMake의 find_package(OpenGL)로 찾아진 OpenGL 헤더 파일 경로
) 