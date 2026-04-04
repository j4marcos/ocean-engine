# ocean-engine
wormhole simulation with ray-casting

demonstração do ray cast em 2d 

g++ raycast/warp2d.cpp -std=c++17 -O2 -Wall -Wextra -pedantic -lGL -lGLU -lglut -o raycast/warp2d_demo
./raycast/warp2d_demo

worm space 3d 

build wormhole 3d demo (contexto OpenGL 2.1; raycast GPU usa GLSL 1.20 + framebuffer object / blit)
g++ raycast/wormhole3d_globals.cpp raycast/wormhole3d_simulation.cpp raycast/wormhole3d_init.cpp raycast/wormhole3d_raster.cpp raycast/wormhole3d_raycast_cpu.cpp raycast/wormhole3d_raycast_gpu.cpp raycast/wormhole3d_raycast.cpp raycast/wormhole3d_ui.cpp raycast/wormhole3d_main.cpp -std=c++17 -O2 -Wall -Wextra -pedantic -lGL -lGLU -lglut -ldl -o raycast/wormhole3d_demo

Ray marching é uma técnica de renderização 3D baseada em Signed Distance Functions (SDFs), onde raios são disparados da câmera e progridem iterativamente em passos, calculando a menor distância até um objeto para avançar com segurança, evitando intersecções geométricas diretas