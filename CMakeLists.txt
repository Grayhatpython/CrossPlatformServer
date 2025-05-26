cmake_minimum_required(VERSION 3.10)
project(CrossPlatformServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 외부 라이브러리(GLFW) 가져오기 설정 ---
# FetchContent 모듈을 포함
# FetchContent는 CMake가 빌드 시점에 외부 프로젝트를 다운로드하고 빌드에 통합할 수 있게 해주는 기능
include(FetchContent)

# GLFW 가져오기
FetchContent_Declare(
    glfw                                                # 가져올 외부 프로젝트의 이름 (CMake 내부에서 사용될 이름)
    GIT_REPOSITORY https://github.com/glfw/glfw.git     # GLFW 소스 코드가 있는 Git 저장소 URL
    GIT_TAG 3.3.8                                       # 가져올 GLFW 소스 코드의 특정 버전 (Git 태그 또는 커밋 해시)
)
# 선언된 GLFW 프로젝트를 실제로 다운로드하고, 빌드 시스템에 포함시켜 사용할 수 있도록 만든다.
# 이 명령어가 실행되면 GLFW 소스 코드가 CMake의 빌드 디렉토리 내부에 다운로드
FetchContent_MakeAvailable(glfw)

# GLFW 빌드 옵션을 설정
# 불필요한 빌드 요소들을 비활성화하여 빌드 시간을 줄이고 최종 바이너리 크기를 최적화
# CACHE BOOL "" FORCE: 캐시 변수로 설정하여 값을 강제하고, 사용자에게 노출하지 않는다.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)        # GLFW 문서 빌드 비활성화
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)       # GLFW 테스트 코드 빌드 비활성화
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)    # GLFW 예제 코드 빌드 비활성화
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)           # GLFW 설치 규칙 비활성화 (프로젝트 내에서만 사용)
# --- 외부 라이브러리(GLFW) 가져오기 설정  ---


# 시스템에 설치된 OpenGL 라이브러리를 찾습니다.
# REQUIRED 키워드는 OpenGL 라이브러리를 찾지 못하면 빌드 오류를 발생
# 이 명령어가 성공하면 ${OPENGL_LIBRARIES}, ${OPENGL_INCLUDE_DIR} 등의 변수가 CMake에 의해 설정
find_package(OpenGL REQUIRED)

if (MSVC)
    # 모든 C++ 컴파일에 /utf-8 플래그를 추가합니다.
    add_compile_options(/utf-8)
    
    add_compile_definitions(UNICODE _UNICODE)
endif()

# 'ServerCore' 하위 디렉토리에 있는 CMakeLists.txt 파일을 포함시키고 처리
# 이 명령으로 ServerCore 라이브러리가 빌드 설정에 추가
add_subdirectory(ServerCore)

# 'Server' 하위 디렉토리에 있는 CMakeLists.txt 파일을 포함시키고 처리
# 이 명령으로 Server 실행 파일이 빌드 설정에 추가됩니다.
add_subdirectory(Server) 

# 'MonitoringServer' 하위 디렉토리에 있는 CMakeLists.txt 파일을 포함시키고 처리
# 이 명령으로 Server 실행 파일이 빌드 설정에 추가됩니다.
add_subdirectory(MonitoringServer) 

# 'DummyClient' 하위 디렉토리에 있는 CMakeLists.txt 파일을 포함시키고 처리
# 이 명령으로 Server 실행 파일이 빌드 설정에 추가됩니다.
add_subdirectory(DummyClient) 


# --- 빌드 명령어 가이드 ---
# 프로젝트 구성(Configure) 및 빌드(Build)를 위한 커맨드 라인 명령어 예시입니다.
# 이는 실제 CMake 파일의 명령어가 아니라, 개발자가 터미널에서 입력할 명령어입니다.

# 1. 빌드 디렉토리 생성 및 CMake 프로젝트 구성 (Configure):
#    -Bbuild: 'build'라는 이름의 빌드 디렉토리를 생성하고 모든 빌드 파일을 그 안에 생성합니다.
#    .: 현재 디렉토리(루트 CMakeLists.txt가 있는 곳)를 소스 디렉토리로 지정합니다.
#    -DCMAKE_BUILD_TYPE=Debug|Release: 빌드 타입을 Debug 또는 Release로 설정합니다. (생략 시 기본값 사용)
# cmake -Bbuild . -DCMAKE_BUILD_TYPE=Debug|Release

# 2. 프로젝트 빌드:
#    --build build: 'build' 디렉토리에 생성된 빌드 시스템(Makefile, Visual Studio 등)을 사용하여 프로젝트를 실제로 빌드합니다.
# cmake --build build