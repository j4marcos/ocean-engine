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


metas pra finalização:

ilha com montanhas criadas com textura de height-bump -> criar relevo sem precisar modelar -> textura de grama
mar e aguã com propriedade reflexiva -> raios so refletem dando uma tonalidade azul
arvores low poly -> opcional pra dar um verde
predios com luzes -> pra chamar atenção do raytracing -> e predios tem textura de fachadas 
ruas -> com textura de dua
postes com luzes -> pra chamar atenção do raytracing

no meio dos predios o burado A -> a cidade vê o portal pra o espaço 

no ceu o buraco B -> o espaço vê o portal pra a cidade

passaros voando no ceu em loop -> curvas de besier


calculo de raycast: 

no cpu:
joga raio de luz ate intercectar algo, se interceptar objeto , calcula se o caminho ate um fonte de luz esta livre (oclusão)
se o raio cair em um wormhole warp radios então o raio se conporta como um ray march. 

no gpu:


// ilha (superfície 3d com heightmap usando real-bumpmap)

// predios (array de objetos predio) (bloco 3d com textura de paredes)

// arvores (array de objetos arvore) (tronco 3d)

// folhas (array de objetos folha) (textura de folha em 8 direções juntas)

// poste (aste 3d com ponto de luz no topo)

// agua do mar (superfície gigante 3d com heightmap usando real-bumpmap) (totalmente refletiva, mas deixa o raio um pouco azul)
