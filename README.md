# Ocean Engine 🌊

Uma engine de jogos 3D simples e orientada a objetos, construída com OpenGL.

## 🎯 Objetivo

Ocean Engine foi projetada para ser uma engine de jogos minimalista e educacional, focando em:
- **Programação Orientada a Objetos** clara e intuitiva
- **Simplicidade** sobre complexidade
- **OpenGL** como base gráfica

## 📁 Estrutura do Projeto

```
ocean-engine/
├── include/           # Headers da engine
│   ├── OceanEngine.hpp   # Header principal (inclui tudo)
│   ├── Math3D.hpp        # Point, Direction, RGB, Rotation
│   ├── Element.hpp       # Classe base para objetos
│   ├── Shape.hpp         # Formas 2D (Circle, Ring)
│   ├── Form.hpp          # Formas 3D (Sphere, Box, Torus)
│   ├── Entity.hpp        # Sistema de entidades (Entity, Body, Player)
│   ├── Camera.hpp        # Sistema de câmera
│   ├── Renderer.hpp      # Renderizador e iluminação
│   ├── Window.hpp        # Gerenciamento de janela
│   └── Scene.hpp         # Gerenciamento de cenas
├── src/               # Código fonte
│   └── main.cpp          # Exemplo: Fantasy World
├── CMakeLists.txt     # Sistema de build
├── build.sh           # Script de build
└── README.md
```

## 🏗️ Arquitetura

### Hierarquia de Classes

```
Element (base para tudo no espaço 3D)
├── Shape (figuras 2D)
│   ├── Circle
│   └── Ring
└── Form (formas 3D)
    ├── Sphere
    ├── Box
    ├── Torus
    └── Body (corpo físico de entidades)

Entity (seres vivos)
└── Player (controlado pelo usuário)

Camera (visualização)
Scene (gerenciamento de cenas)
Renderer (renderização OpenGL)
Window (janela/contexto GLUT)
```

## 🔧 Dependências

- **OpenGL** - Biblioteca gráfica
- **GLUT/FreeGLUT** - Gerenciamento de janela
- **GLU** - Utilitários OpenGL
- **CMake** (3.10+) - Sistema de build

### Instalação das Dependências (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake
sudo apt install libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev
```

### Instalação das Dependências (Fedora)

```bash
sudo dnf install gcc-c++ cmake
sudo dnf install mesa-libGL-devel mesa-libGLU-devel freeglut-devel
```

### Instalação das Dependências (Arch Linux)

```bash
sudo pacman -S base-devel cmake
sudo pacman -S mesa glu freeglut
```

## 🚀 Compilação e Execução

### Usando o script (recomendado)

```bash
chmod +x build.sh
./build.sh
```

### Manualmente com CMake

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
./ocean_engine
```

## 🎮 Controles (Fantasy World Demo)

| Tecla | Ação |
|-------|------|
| W/S | Mover frente/trás |
| A/D | Mover esquerda/direita |
| Q/E | Descer/Subir |
| Mouse | Olhar ao redor (após clicar) |
| ESC | Desbloquear mouse / Sair |
| L | Alternar wireframe |

## 📝 Exemplo de Uso

```cpp
#include "OceanEngine.hpp"
using namespace Ocean;

int main(int argc, char** argv) {
    // Cria janela
    Window window(1280, 720, "Meu Jogo");
    window.init(&argc, argv);
    
    // Cria cena
    Scene scene("Minha Cena");
    
    // Cria uma esfera
    Sphere* sol = scene.createElement<Sphere>();
    sol->setPosition(Point(0, 10, 0))
       .setRadius(5.0f)
       .setColor(RGB::yellow());
    
    // Cria player
    Player* player = scene.createEntity<Player>();
    player->setPosition(Point(0, 0, 20));
    
    // Configura câmera para seguir o player
    scene.getCamera()
         .follow(player->getBody())
         .lookAt(player->getBody());
    
    // Inicia o loop
    scene.init();
    window.onDisplay = [&]() { scene.render(); };
    window.onIdle = [&]() { scene.update(0.016f); };
    window.run();
    
    return 0;
}
```

## 🔮 Recursos Futuros

- [ ] Sistema de física básica
- [ ] Carregamento de modelos 3D
- [ ] Sistema de partículas
- [ ] Texturas
- [ ] Som
- [ ] Sistema de colisão
- [ ] UI/HUD

## 📄 Licença

Este projeto é de código aberto e pode ser usado livremente para fins educacionais e pessoais.

---

Feito com ❤️ para aprender OpenGL e desenvolvimento de games