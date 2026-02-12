cmake_minimum_required(VERSION 3.10)
project(OceanEngine VERSION 1.0.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opções de compilação
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Encontra OpenGL e GLUT
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Diretório de includes
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
)

# Coleta todos os arquivos fonte
file(GLOB ENGINE_SOURCES 
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# Cria o executável
add_executable(ocean_engine ${ENGINE_SOURCES})

# Linka as bibliotecas
target_link_libraries(ocean_engine
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    GLU
    m
)

# Instalação (opcional)
install(TARGETS ocean_engine DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/ocean)

# Mensagens de configuração
message(STATUS "OpenGL found: ${OPENGL_LIBRARIES}")
message(STATUS "GLUT found: ${GLUT_LIBRARIES}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
