#ifndef SCENE_HPP
#define SCENE_HPP

#include "Renderer.hpp"
#include "Window.hpp"
#include "Entity.hpp"
#include <vector>
#include <memory>

namespace Ocean {

/**
 * @brief Classe Scene gerencia uma cena completa do jogo
 */
class Scene {
protected:
    std::string name;
    Renderer renderer;
    Camera camera;
    std::vector<std::unique_ptr<Element>> ownedElements;
    std::vector<std::unique_ptr<Entity>> entities;
    bool paused;
    float timeScale;

public:
    Scene(const std::string& sceneName = "Scene")
        : name(sceneName)
        , paused(false)
        , timeScale(1.0f)
    {
        renderer.setCamera(&camera);
    }

    virtual ~Scene() = default;

    // Getters
    std::string getName() const { return name; }
    Renderer& getRenderer() { return renderer; }
    Camera& getCamera() { return camera; }
    bool isPaused() const { return paused; }
    float getTimeScale() const { return timeScale; }

    // Setters
    void setName(const std::string& n) { name = n; }
    void setPaused(bool p) { paused = p; }
    void setTimeScale(float scale) { timeScale = scale; }

    // Adiciona um elemento à cena (transfere ownership)
    template<typename T, typename... Args>
    T* createElement(Args&&... args) {
        auto element = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = element.get();
        renderer.addElement(ptr);
        ownedElements.push_back(std::move(element));
        return ptr;
    }

    // Adiciona uma entidade à cena
    template<typename T, typename... Args>
    T* createEntity(Args&&... args) {
        auto entity = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = entity.get();
        if (ptr->getBody()) {
            renderer.addElement(ptr->getBody());
        }
        entities.push_back(std::move(entity));
        return ptr;
    }

    // Adiciona elemento existente (não transfere ownership)
    void addElement(Element* element) {
        renderer.addElement(element);
    }

    // Métodos virtuais para override
    virtual void init() {
        renderer.init();
    }

    virtual void update(float deltaTime) {
        if (paused) return;

        float scaledDelta = deltaTime * timeScale;

        // Atualiza entidades
        for (auto& entity : entities) {
            entity->update(scaledDelta);
        }

        // Atualiza renderer (que atualiza elementos e câmera)
        renderer.update(scaledDelta);
    }

    virtual void render() {
        renderer.render();
    }

    virtual void processInput(int key, bool pressed) {
        // Processa input para todas as entidades
        for (auto& entity : entities) {
            entity->processInput(key, pressed);
        }
    }

    virtual void processSpecialKey(int key, bool pressed) {
        // Processa teclas especiais (setas, F1-F12, etc)
        // Pode ser sobrescrito por classes derivadas
    }

    virtual void onEnter() {
        // Chamado quando a cena se torna ativa
    }

    virtual void onExit() {
        // Chamado quando a cena deixa de ser ativa
    }
};

} // namespace Ocean

#endif // SCENE_HPP
