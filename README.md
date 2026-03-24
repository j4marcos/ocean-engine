# ocean-engine
wormhole simulation with ray-casting




warp field - distorcer o espaço

build warp 2d demo : 
g++ raycast/test.cpp -std=c++17 -Wall -Wextra -pedantic -lGL -lGLU -lglut -o raycast/warp_demo



worp space 3d 

build wormhole 3d demo 
g++ raycast/wormhole3d.cpp -std=c++17 -O2 -Wall -Wextra -pedantic -lGL -lGLU -lglut -o raycast/wormhole3d_demo

Ray marching é uma técnica de renderização 3D baseada em Signed Distance Functions (SDFs), onde raios são disparados da câmera e progridem iterativamente em passos, calculando a menor distância até um objeto para avançar com segurança, evitando intersecções geométricas diretas