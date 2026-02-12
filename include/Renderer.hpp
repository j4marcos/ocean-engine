#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "Element.hpp"
#include "Camera.hpp"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <vector>
#include <memory>
#include <algorithm>

namespace Ocean {

/**
 * @brief Classe Light para iluminação da cena
 */
class Light {
private:
    GLenum lightId;
    Point position;
    RGBA ambient;
    RGBA diffuse;
    RGBA specular;
    bool enabled;
    bool isDirectional;

public:
    Light(GLenum id = GL_LIGHT0)
        : lightId(id)
        , position(0.0f, 10.0f, 0.0f)
        , ambient(0.2f, 0.2f, 0.2f)
        , diffuse(1.0f, 1.0f, 1.0f)
        , specular(1.0f, 1.0f, 1.0f)
        , enabled(true)
        , isDirectional(false)
    {}

    Light& setPosition(const Point& pos) {
        position = pos;
        return *this;
    }

    Light& setAmbient(const RGBA& color) {
        ambient = color;
        return *this;
    }

    Light& setDiffuse(const RGBA& color) {
        diffuse = color;
        return *this;
    }

    Light& setSpecular(const RGBA& color) {
        specular = color;
        return *this;
    }

    Light& setEnabled(bool e) {
        enabled = e;
        return *this;
    }

    Light& setDirectional(bool d) {
        isDirectional = d;
        return *this;
    }

    void apply() {
        if (enabled) {
            glEnable(lightId);
        } else {
            glDisable(lightId);
            return;
        }

        float pos[] = { position.x, position.y, position.z, isDirectional ? 0.0f : 1.0f };
        float amb[] = { ambient.r, ambient.g, ambient.b, 1.0f };
        float dif[] = { diffuse.r, diffuse.g, diffuse.b, 1.0f };
        float spec[] = { specular.r, specular.g, specular.b, 1.0f };

        glLightfv(lightId, GL_POSITION, pos);
        glLightfv(lightId, GL_AMBIENT, amb);
        glLightfv(lightId, GL_DIFFUSE, dif);
        glLightfv(lightId, GL_SPECULAR, spec);
    }

    Point getPosition() const { return position; }
};

/**
 * @brief Renderizador principal da engine
 */
class Renderer {
private:
    Camera* activeCamera;
    std::vector<Element*> elements;
    std::vector<Light> lights;
    RGBA clearColor;
    RGBA ambientLight;
    bool wireframeMode;
    bool lightingEnabled;

public:
    Renderer()
        : activeCamera(nullptr)
        , clearColor(0.0f, 0.0f, 0.1f)
        , ambientLight(0.1f, 0.1f, 0.1f)
        , wireframeMode(false)
        , lightingEnabled(true)
    {
        // Adiciona uma luz padrão
        lights.push_back(Light(GL_LIGHT0));
    }

    // Configuração da câmera
    Renderer& setCamera(Camera* cam) {
        activeCamera = cam;
        return *this;
    }

    Camera* getCamera() { return activeCamera; }

    // Gerenciamento de elementos
    Renderer& addElement(Element* element) {
        elements.push_back(element);
        return *this;
    }

    Renderer& removeElement(Element* element) {
        auto it = std::find(elements.begin(), elements.end(), element);
        if (it != elements.end()) {
            elements.erase(it);
        }
        return *this;
    }

    void clearElements() {
        elements.clear();
    }

    // Gerenciamento de luzes
    Light& addLight(GLenum id = GL_LIGHT0) {
        lights.push_back(Light(id));
        return lights.back();
    }

    Light& getLight(size_t index) {
        return lights[index];
    }

    // Configurações
    Renderer& setClearColor(const RGBA& color) {
        clearColor = color;
        return *this;
    }

    Renderer& setAmbientLight(const RGBA& color) {
        ambientLight = color;
        return *this;
    }

    Renderer& setWireframeMode(bool enabled) {
        wireframeMode = enabled;
        return *this;
    }

    Renderer& setLightingEnabled(bool enabled) {
        lightingEnabled = enabled;
        return *this;
    }

    // Inicializa o renderer
    void init() {
        // Configurações básicas do OpenGL
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        
        if (lightingEnabled) {
            glEnable(GL_LIGHTING);
            glEnable(GL_COLOR_MATERIAL);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        }

        glShadeModel(GL_SMOOTH);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

        // Configura luz ambiente global
        float ambient[] = { ambientLight.r, ambientLight.g, ambientLight.b, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    }

    // Limpa o frame
    void clear() {
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // Renderiza a cena
    void render() {
        // Limpa buffers
        clear();

        // Aplica a câmera
        if (activeCamera) {
            activeCamera->apply();
        } else {
            // Configuração padrão se não houver câmera
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluPerspective(45.0, 1.333, 0.1, 1000.0);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            gluLookAt(0, 5, 20, 0, 0, 0, 0, 1, 0);
        }

        // Configura iluminação
        if (lightingEnabled) {
            glEnable(GL_LIGHTING);
            for (auto& light : lights) {
                light.apply();
            }
        } else {
            glDisable(GL_LIGHTING);
        }

        // Configura modo de renderização
        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // Renderiza todos os elementos
        for (auto* element : elements) {
            if (element) {
                element->render();
            }
        }

        // Troca buffers
        glutSwapBuffers();
    }

    // Atualiza todos os elementos
    void update(float deltaTime) {
        for (auto* element : elements) {
            if (element) {
                element->update(deltaTime);
            }
        }

        if (activeCamera) {
            activeCamera->update(deltaTime);
        }
    }

    // Desenha um grid no chão (útil para debug)
    void drawGrid(float size, int divisions) {
        glDisable(GL_LIGHTING);
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINES);
        
        float step = size / divisions;
        float halfSize = size / 2.0f;
        
        for (int i = 0; i <= divisions; ++i) {
            float pos = -halfSize + i * step;
            
            // Linhas paralelas ao eixo X
            glVertex3f(-halfSize, 0.0f, pos);
            glVertex3f(halfSize, 0.0f, pos);
            
            // Linhas paralelas ao eixo Z
            glVertex3f(pos, 0.0f, -halfSize);
            glVertex3f(pos, 0.0f, halfSize);
        }
        
        glEnd();
        
        if (lightingEnabled) {
            glEnable(GL_LIGHTING);
        }
    }

    // Desenha eixos de coordenadas (útil para debug)
    void drawAxes(float length) {
        glDisable(GL_LIGHTING);
        glBegin(GL_LINES);
        
        // Eixo X - Vermelho
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(length, 0.0f, 0.0f);
        
        // Eixo Y - Verde
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, length, 0.0f);
        
        // Eixo Z - Azul
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, length);
        
        glEnd();
        
        if (lightingEnabled) {
            glEnable(GL_LIGHTING);
        }
    }
};

} // namespace Ocean

#endif // RENDERER_HPP
