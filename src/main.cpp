/**
 * @file main.cpp
 * @brief Demonstração da Ocean Engine - Mundo de Fantasia
 * 
 * Este exemplo cria uma cena espacial com uma estrela, um planeta com anel,
 * e um jogador controlável com câmera em terceira pessoa.
 */

#include "../include/OceanEngine.hpp"
#include <iostream>
#include <chrono>

using namespace Ocean;

// ============================================================================
// Cena do Mundo de Fantasia
// ============================================================================

class FantasyWorld : public Scene {
private:
    // Elementos do mundo
    Sphere* star;
    Sphere* planet;
    Torus* planetRing;
    
    // Player
    Player* player;
    
    // Controle de tempo
    float worldTime;
    float starRotation;
    float planetOrbitAngle;
    
    // Mouse
    int lastMouseX, lastMouseY;
    bool firstMouse;
    bool mouseLocked;

public:
    FantasyWorld() 
        : Scene("Fantasy World")
        , star(nullptr)
        , planet(nullptr)
        , planetRing(nullptr)
        , player(nullptr)
        , worldTime(0.0f)
        , starRotation(0.0f)
        , planetOrbitAngle(0.0f)
        , lastMouseX(400)
        , lastMouseY(300)
        , firstMouse(true)
        , mouseLocked(false)
    {}

    void init() override {
        Scene::init();

        // Configura renderização
        renderer.setClearColor(RGBA(0.0f, 0.0f, 0.05f)); // Céu escuro espacial
        renderer.setAmbientLight(RGBA(0.1f, 0.1f, 0.15f));

        // ====================================================================
        // Criar Estrela (Sol)
        // ====================================================================
        star = createElement<Sphere>();
        star->setPosition(Point(0.0f, 0.0f, 0.0f));
        star->setRadius(10.0f);
        star->setColor(RGBA(1.0f, 0.9f, 0.3f));  // Amarelo brilhante
        star->setSlices(32);
        star->setStacks(32);

        // Configura a luz principal na posição da estrela
        Light& sunLight = renderer.getLight(0);
        sunLight.setPosition(star->getPosition())
                .setDiffuse(RGBA(1.0f, 0.95f, 0.8f))
                .setAmbient(RGBA(0.3f, 0.25f, 0.1f));

        // ====================================================================
        // Criar Planeta
        // ====================================================================
        planet = createElement<Sphere>();
        planet->setPosition(Point(50.0f, 0.0f, 0.0f));
        planet->setRadius(5.0f);
        planet->setColor(RGBA(0.3f, 0.5f, 0.8f));  // Azul como a Terra
        planet->setSlices(24);
        planet->setStacks(24);

        // ====================================================================
        // Criar Anel do Planeta (estilo Saturno)
        // ====================================================================
        planetRing = createElement<Torus>();
        planetRing->setPosition(planet->getPosition());
        planetRing->setOuterRadius(planet->getRadius() * 2.0f);  // Anel bem largo
        planetRing->setInnerRadius(3.00f);  // Tubo muito fino, como um disco
        planetRing->setColor(RGBA(0.7f, 0.6f, 0.5f, 0.8f));  // Marrom claro
        planetRing->setRotation(0.3f, 0.0f, 0.0f);  // Inclinação
        planetRing->setSlices(64);  // Suave ao redor do anel
        planetRing->setStacks(2);   // Poucas divisões no tubo para manter fino

        // ====================================================================
        // Criar Player
        // ====================================================================
        player = createEntity<Player>();
        player->setPosition(Point(30.0f, 5.0f, 30.0f));
        player->setMoveSpeed(15.0f);
        player->getBody()->setColor(RGBA(0.2f, 0.8f, 0.4f));  // Verde
        player->getBody()->setRadius(1.0f);

        // ====================================================================
        // Configurar Câmera
        // ====================================================================
        camera.setPosition(Point(30.0f, 10.0f, 45.0f));
        camera.setFollowOffset(Point(0.0f, 5.0f, 15.0f));
        camera.setFOV(60.0f);
        camera.setProjection(60.0f, 800.0f / 600.0f, 0.1f, 500.0f);
        
        camera.follow(player->getBody());
        camera.lookAt(player->getBody());

        // ====================================================================
        // Criar algumas estrelas de fundo (esferas pequenas distantes)
        // ====================================================================
        for (int i = 0; i < 50; ++i) {
            Sphere* bgStar = createElement<Sphere>();
            float angle = (float)i / 50.0f * 2.0f * M_PI;
            float dist = 200.0f + (i % 5) * 50.0f;
            float height = ((i % 10) - 5) * 30.0f;
            
            bgStar->setPosition(Point(
                std::cos(angle) * dist,
                height,
                std::sin(angle) * dist
            ));
            bgStar->setRadius(0.5f + (i % 3) * 0.3f);
            bgStar->setColor(RGBA(1.0f, 1.0f, 0.9f));
            bgStar->setSlices(8);
            bgStar->setStacks(8);
        }

        std::cout << "=== Fantasy World Initialized ===" << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  WASD - Move" << std::endl;
        std::cout << "  Q/E  - Up/Down" << std::endl;
        std::cout << "  Arrow Keys - Rotate camera" << std::endl;
        std::cout << "  Page Up/Down - Zoom camera" << std::endl;
        std::cout << "  Mouse - Look around (click to lock)" << std::endl;
        std::cout << "  ESC  - Unlock mouse / Exit" << std::endl;
        std::cout << "  G    - Toggle grid" << std::endl;
        std::cout << "  L    - Toggle wireframe" << std::endl;
        std::cout << "=================================" << std::endl;
    }

    void update(float deltaTime) override {
        Scene::update(deltaTime);

        worldTime += deltaTime;

        // Rotação da estrela
        starRotation += deltaTime * 0.1f;
        if (star) {
            star->setRotation(0.0f, starRotation, 0.0f);
        }

        // Órbita do planeta ao redor da estrela
        planetOrbitAngle += deltaTime * 0.05f;
        if (planet && planetRing) {
            float orbitRadius = 50.0f;
            Point newPos(
                std::cos(planetOrbitAngle) * orbitRadius,
                0.0f,
                std::sin(planetOrbitAngle) * orbitRadius
            );
            planet->setPosition(newPos);
            planetRing->setPosition(newPos);
            
            // Rotação própria do planeta
            Rotation rot = planet->getRotation();
            rot.yaw += deltaTime * 0.5f;
            planet->setRotation(rot);
        }
    }

    void processInput(int key, bool pressed) override {
        Scene::processInput(key, pressed);

        if (pressed) {
            switch (key) {
                case 27: // ESC
                    if (mouseLocked) {
                        mouseLocked = false;
                        std::cout << "Mouse unlocked" << std::endl;
                    } else {
                        exit(0);
                    }
                    break;
                case 'g': case 'G':
                    // Toggle grid seria implementado aqui
                    break;
                case 'l': case 'L':
                    // Toggle wireframe
                    static bool wireframe = false;
                    wireframe = !wireframe;
                    renderer.setWireframeMode(wireframe);
                    std::cout << "Wireframe: " << (wireframe ? "ON" : "OFF") << std::endl;
                    break;
            }
        }
    }

    void processSpecialKey(int key, bool pressed) override {
        Scene::processSpecialKey(key, pressed);

        if (pressed) {
            switch (key) {
                // Controles de rotação da câmera (setas)
                case GLUT_KEY_LEFT:
                    camera.rotate(0.1f, 0.0f);  // Rotaciona para esquerda
                    break;
                case GLUT_KEY_RIGHT:
                    camera.rotate(-0.1f, 0.0f); // Rotaciona para direita
                    break;
                case GLUT_KEY_UP:
                    camera.rotate(0.0f, 0.1f);  // Rotaciona para cima
                    break;
                case GLUT_KEY_DOWN:
                    camera.rotate(0.0f, -0.1f); // Rotaciona para baixo
                    break;
                
                // Zoom com Page Up/Down
                case GLUT_KEY_PAGE_UP:
                    camera.zoom(-1.0f);  // Aproxima
                    break;
                case GLUT_KEY_PAGE_DOWN:
                    camera.zoom(1.0f);   // Afasta
                    break;
            }
        }
    }

    void render() override {
        Scene::render();
        
        // Desenha grid de debug
        renderer.drawGrid(200.0f, 20);
        renderer.drawAxes(10.0f);
    }
};

// ============================================================================
// Aplicação Principal
// ============================================================================

class Application {
private:
    Window window;
    FantasyWorld world;
    
    // Controle de tempo
    std::chrono::high_resolution_clock::time_point lastTime;
    float deltaTime;

public:
    Application() : deltaTime(0.0f) {}

    bool init(int* argc, char** argv) {
        // Inicializa janela
        if (!window.init(argc, argv)) {
            return false;
        }

        window.setTitle("Ocean Engine - Fantasy World");
        window.setSize(1280, 720);

        // Inicializa cena
        world.init();
        world.getCamera().setAspectRatio(window.getAspectRatio());

        // Configura callbacks
        window.onDisplay = [this]() {
            this->render();
        };

        window.onReshape = [this](int w, int h) {
            world.getCamera().setAspectRatio(
                static_cast<float>(w) / static_cast<float>(h)
            );
        };

        window.onKeyPress = [this](int key, bool pressed) {
            world.processInput(key, pressed);
        };

        window.onSpecialKey = [this](int key, bool pressed) {
            world.processSpecialKey(key, pressed);
        };

        window.onIdle = [this]() {
            this->update();
        };

        lastTime = std::chrono::high_resolution_clock::now();

        return true;
    }

    void update() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Limita deltaTime para evitar problemas
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        world.update(deltaTime);
    }

    void render() {
        world.render();
    }

    void run() {
        window.run();
    }
};

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "       Ocean Engine - Fantasy World    " << std::endl;
    std::cout << "========================================" << std::endl;

    Application app;

    if (!app.init(&argc, argv)) {
        std::cerr << "Failed to initialize application!" << std::endl;
        return 1;
    }

    app.run();

    return 0;
}
