# Wormhole Raycasting Simulation

Simulação 3D de wormholes com raycasting/ray marching, renderização por GPU (GLSL shaders) e rasterização OpenGL clássica como fallback.

**Disciplina:** Computação Gráfica — 2025-2

**Equipe:**
- Antônio Augusto Dantas Neto
- João Marcos Cunha Santos
- Kaique Bezerra Santos

## Visão Geral

O projeto simula um par de bocas de wormhole conectadas por um campo de deformação gravitacional. Raios de luz são disparados da câmera e curvam-se ao se aproximar das bocas, com teleporte automático entre elas. A cena inclui iluminação Lambert, SDFs para geometria (esferas, cubos, floor) e skybox.

### Técnicas utilizadas

| Técnica | Descrição |
|---|---|
| **Ray marching (GPU)** | Fragment shader GLSL 1.20 com FBO + blit — cada pixel dispara raios que marcham via SDFs |
| **Ray marching (CPU)** | Fallback por software com `glDrawPixels` |
| **Warp field** | Campo vetorial gaussiano que desvia os raios, simulando a curvatura do espaço-tempo |
| **Teleporte SDF** | Quando um raio cruza o raio de uma boca, é teleportado para a boca oposta |
| **Bézier cúbica** | Animação da câmera ao longo de uma curva de Bézier P0→P1→P2→P3 |
| **Skybox esférica** | Domo texturizado (stars.jpg) que segue a câmera |
| **Iluminação Lambert** | Ambiente + difusa com normais estimadas via gradiente numérico do SDF |

## Estrutura do Projeto

```
ocean-engine/
├── wormhole-scene/          # Código principal (modularizado)
│   ├── Makefile             # Build system
│   ├── wormhole3d_main.cpp  # Entry point + loop GLUT
│   ├── audio/               # Áudio de fundo (ALURE)
│   ├── scene/               # Prefabs, geometria, movimentação
│   ├── simulation/          # Física do campo de warp / teleporte
│   ├── raycast/             # Ray marching GPU (shaders) + CPU
│   ├── raster/              # Rasterização OpenGL (fallback)
│   ├── overlay/             # UI sobreposta (modo, FPS)
│   ├── texture/             # Carregamento de texturas (stb_image)
│   └── assets/              # Texturas e áudio
├── wormhole-2d/
│   └── warp2d.cpp           # Demo 2D — raios em campo de deformação
└── praticas/                # PDFs das aulas práticas de CG
```

## Build & Run

### Demos 3D

```bash
cd wormhole-scene
make
./wormhole3d_demo
```

### Demo 2D

```bash
g++ wormhole-2d/warp2d.cpp -std=c++17 -O2 -Wall -Wextra -lGL -lGLU -lglut -o wormhole-2d/warp2d_demo
./wormhole-2d/warp2d_demo
```

### Dependências

- **C++17**, **OpenGL 2.1**, **GLSL 1.20**
- `libGL`, `libGLU`, `freeglut`, `libdl`
- `stb_image` (inclusa) para carregamento de texturas

## Compatibilidade entre Distribuições Linux

### Dependências por distribuição

**Ubuntu / Debian:**

```bash
sudo apt install build-essential freeglut3-dev libglu1-mesa-dev
```

**Fedora:**

```bash
sudo dnf install gcc-c++ freeglut-devel mesa-libGLU-devel
```

**Arch Linux / Manjaro:**

```bash
sudo pacman -S gcc freeglut glu
```

### Problemas conhecidos e adaptações

**`glutSwapInterval` e `glutInitContextVersion` não disponíveis**

Essas funções são extensões do **freeglut 3.x** e podem não estar presentes em distribuições que empacotam versões mais antigas ou o GLUT clássico. Se a compilação falhar com erros como:

```
error: 'glutSwapInterval' was not declared in this scope
error: 'glutInitContextVersion' was not declared in this scope
```

comente ou remova as seguintes linhas em `wormhole3d_main.cpp`:

```cpp
// glutInitContextVersion(2, 1);  // extensão freeglut 3.x
// glutSwapInterval(0);            // extensão freeglut 3.x
```

Também remova o `#include <GL/freeglut_ext.h>` se presente.

**Erro de linkagem (`undefined reference`)**

Em linkers modernos (GNU ld recentes), a ordem dos argumentos no link importa: bibliotecas devem vir **depois** dos objetos. Se aparecerem erros como `undefined reference to 'glViewport'`, certifique-se de que o Makefile usa:

```makefile
$(CXX) -o $@ $(OBJS) $(LDFLAGS)    # correto
$(CXX) $(LDFLAGS) -o $@ $(OBJS)    # pode falhar
```

**Driver de vídeo e OpenGL**

É necessário um driver com suporte a **OpenGL 2.1** ou superior (qualquer driver Intel, AMD ou NVIDIA recente). Em máquinas virtuais sem aceleração 3D, o raycasting GPU falhará e o programa usará o fallback por CPU automaticamente.

### Resumo das modificações de portabilidade

```
Arquivo                │ Modificação                                     │ Motivo
───────────────────────┼─────────────────────────────────────────────────┼───────────────────────────
wormhole3d_main.cpp    │ Remover glutInitContextVersion / SwapInterval   │ Extensão freeglut 3.x ausente
Makefile               │ $(OBJS) antes de $(LDFLAGS) na regra de link    │ Linker moderno (ordem importa)
```

## Controles

| Tecla | Ação |
|---|---|
| **WASD** | Mover câmera |
| **Z / X** | Descer / subir câmera no eixo Y |
| **Setas** | Olhar ao redor |
| **1** | Rasterização |
| **2** | Raycast com CPU |
| **3** | Raycast com GPU |
| **4** | Alternar animação Bézier (câmera voa pelo wormhole) |
| **R / F** | Aumentar / diminuir força do warp |
| **[ / ]** | Diminuir / ajustar tempo manual do ciclo dia/noite |
| **T** | Alternar ciclo dia/noite automático |
| **V** | Alternar veículos na cena |

## Screenshots

### Rasterização OpenGL (leve)

![Rasterização](screenshots/raster.png)

### Raycasting GPU/GLSL

![Raycast GPU](screenshots/raycast-gpu.png)

### Modo 2D — Campo de deformação

![Warp 2D](screenshots/warp2d.png)

## Discussão

O principal desafio técnico foi refatorar o projeto — originalmente escrito com chamadas OpenGL quase puras em um único arquivo — para uma arquitetura modular que utiliza a GPU para acelerar o cálculo de raycasting via shaders GLSL. Isso exigiu separação de responsabilidades (simulação, renderização, UI, áudio), gerenciamento de uniformes no fragment shader e fallback por CPU quando recursos de GPU limitados não estão disponíveis.

Durante o desenvolvimento, foram consolidados conceitos de física aplicados à simulação de wormholes (campo gravitacional com suavização gaussiana, teleporte entre bocas) e técnicas práticas de iluminação baseada em normais derivadas numericamente de SDFs. O projeto também serviu como aplicação real de Bézier para animação de câmera, mapeamento de texturas e gerenciamento de contexto OpenGL.

## Metas Futuras

- Cena de ilha com montanhas procedurais via heightmap (bump mapping) e textura de grama
- Superfície de mar/água reflexiva com tonalidade azul
- Cenário urbano com prédios texturizados, ruas, postes luminosos e árvores low-poly
- Buraco A no meio da cidade (portal visível) e Buraco B no céu
- Animacão de pássaros voando em loop via curvas de Bézier
