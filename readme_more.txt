
O que eh wormhole 

Distorção  na 4 dimensão,  ou simular usando campo de Distorção. 

Interacao com a luz, ray marshing e ray cast . Mostrar imagens de comparação 

Estrutura do projeto 
definição dos elementos na scene
elementos prefabs formado de primitivas e elementos especiais
listar elementos e prefabs 
movimentação dos elementos
envio das primiticas pra camada de renderização
Primitivas elementos cubo e esfera

estrutura das variaveis globais

Scena gera elementos e as controla . 3 formas em renderização dos mesmos elementos 

Rasterizacao
Raycast cpu 
Raycast gpu usando GLSL



Raycast 

Distorção 
Luz direcional  ciclo dia noite 
Reflexo plano da agua 
Pontos de luz , poste e farou
Sombras oclusão 
Sky box quando máxima distância 


bvh: otimização com estrutura espacial bvh , mas sem resultado positivo adicional na performance


processo: 

prototipo 2d
imagem do prototipo 2d

prototipo 3d
imagens dos tests dos algotimos de raycast e raymarch 

adicoção de shaders pra gpu 
imagem

adicição de sombras de oclusão 
imagem

adição de reflaxiva
imagem

criação da cena 
imagens

elementos com movimentação
imagens

portais com rasterização 
imagens


wormhole A -> buraco de minhoca no meio da rua 
imagem

praia 
predios -> iteração e textura
carros -> animação com curvas de bezier e 4 pontos de luz
postes -> estrutura e 1 ponto de luz
arvores

imagens

wormhole B -> buraco de minhaca no alto do farol 
imagem

farol -> estrutura com caixa sem uma face que gira e deixa a luz escapar do ponto de luz interno forte
passaros -> esferas com curvas de bezier 
barcos -> postes com curvas e orbita o farol

imagens

+
musica 
texturas 



futuro: 

criar montanhas com superficie gemetrica gerada por textura bumb 
otimizar verificações do raycast


requisitos da atividades Introdução a computação grafica - open gl

Aula 12 Curvas paramétricas.pptx
Aula 11 - Textura.pptx
Aula 10 - Sombreamento.pptx
Aula 09 - Illuminacao.pptx
Aula 01 - Introdução Computação Gráfica 2025-2.pptx
Aula 08 - visibilidade.pptx
Aula 13 - Estruturas de dados espaciais.pptx
Aula 07 - Visualização3D.pptx
Aula 05 - Pipeline de renderizacao.pptx
Aula 03 - LuzCor.pptx
Aula 02 - transformacao.pptx
Aula 14 - Animacao.pptx
Aula 06 - OpenGL.pptx

o que cada membro fez: 
joão marcos -> raycast e cena
antonio -> portal rasterização e textura
kaique -> BVH e ajustes

problemas encontrados: 
animaçao do farol bugada 
animação dos barcos cambaleante
imagem serrilhada de objetos dentro da area do raymach dentro do warp do buraco negro 
repetição de codigo no glsl
