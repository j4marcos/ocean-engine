#ifndef OCEAN_ENGINE_HPP
#define OCEAN_ENGINE_HPP

/**
 * @file OceanEngine.hpp
 * @brief Header principal da Ocean Engine
 * 
 * Inclui todos os componentes da engine para uso facilitado.
 */

// Matemática e tipos básicos
#include "Math3D.hpp"

// Classes base
#include "Element.hpp"

// Formas 2D e 3D
#include "Shape.hpp"
#include "Form.hpp"

// Sistema de entidades
#include "Entity.hpp"

// Câmera
#include "Camera.hpp"

// Renderização
#include "Renderer.hpp"
#include "Window.hpp"

// Gerenciamento de cenas
#include "Scene.hpp"

/**
 * @namespace Ocean
 * @brief Namespace principal da Ocean Engine
 * 
 * A Ocean Engine é uma engine de jogos 3D simples baseada em OpenGL,
 * projetada com foco em programação orientada a objetos e simplicidade.
 * 
 * Hierarquia de classes:
 * 
 * Element (base)
 * ├── Shape (2D)
 * │   ├── Circle
 * │   └── Ring
 * └── Form (3D)
 *     ├── Sphere
 *     ├── Box
 *     ├── Torus
 *     └── Body
 * 
 * Entity (seres vivos)
 * └── Player
 * 
 * Camera (visualização)
 * 
 * Renderer (renderização)
 * Light (iluminação)
 * Window (janela/contexto)
 * Scene (gerenciamento de cenas)
 */

#endif // OCEAN_ENGINE_HPP
