# ocean-engine
wormhole simulation with ray-casting

demonstração do ray cast em 2d 

g++ wormhole-2d/warp2d.cpp -std=c++17 -O2 -Wall -Wextra -pedantic -lGL -lGLU -lglut -o wormhole-2d/warp2d_demo
./wormhole-2d/warp2d_demo

worm space 3d 
wormhole 3d demo (contexto OpenGL 2.1; raycast GPU usa GLSL 1.20 + framebuffer object / blit)

build:
cd wormhole-scene && make 

run: 
./wormhole3d_demo

Ray marching é uma técnica de renderização 3D baseada em Signed Distance Functions (SDFs), onde raios são disparados da câmera e progridem iterativamente em passos, calculando a menor distância até um objeto para avançar com segurança, evitando intersecções geométricas diretas