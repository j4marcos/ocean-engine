#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <string>
#include <functional>
#include <iostream>

namespace Ocean {

/**
 * @brief Classe para gerenciar a janela e contexto OpenGL
 */
class Window {
private:
    int width;
    int height;
    std::string title;
    bool fullscreen;
    int windowHandle;

    // Callbacks estáticos (GLUT requer funções estáticas)
    static Window* currentInstance;
    
    static void displayCallback() {
        if (currentInstance && currentInstance->onDisplay) {
            currentInstance->onDisplay();
        }
    }

    static void reshapeCallback(int w, int h) {
        if (currentInstance) {
            currentInstance->width = w;
            currentInstance->height = h;
            glViewport(0, 0, w, h);
            if (currentInstance->onReshape) {
                currentInstance->onReshape(w, h);
            }
        }
    }

    static void keyboardCallback(unsigned char key, int x, int y) {
        if (currentInstance && currentInstance->onKeyPress) {
            currentInstance->onKeyPress(key, true);
        }
    }

    static void keyboardUpCallback(unsigned char key, int x, int y) {
        if (currentInstance && currentInstance->onKeyPress) {
            currentInstance->onKeyPress(key, false);
        }
    }

    static void specialKeyCallback(int key, int x, int y) {
        if (currentInstance && currentInstance->onSpecialKey) {
            currentInstance->onSpecialKey(key, true);
        }
    }

    static void specialKeyUpCallback(int key, int x, int y) {
        if (currentInstance && currentInstance->onSpecialKey) {
            currentInstance->onSpecialKey(key, false);
        }
    }

    static void mouseCallback(int button, int state, int x, int y) {
        if (currentInstance && currentInstance->onMouseClick) {
            currentInstance->onMouseClick(button, state == GLUT_DOWN, x, y);
        }
    }

    static void motionCallback(int x, int y) {
        if (currentInstance && currentInstance->onMouseMove) {
            currentInstance->onMouseMove(x, y);
        }
    }

    static void passiveMotionCallback(int x, int y) {
        if (currentInstance && currentInstance->onMouseMove) {
            currentInstance->onMouseMove(x, y);
        }
    }

    static void idleCallback() {
        if (currentInstance && currentInstance->onIdle) {
            currentInstance->onIdle();
        }
        glutPostRedisplay();
    }

    static void timerCallback(int value) {
        if (currentInstance && currentInstance->onTimer) {
            currentInstance->onTimer(value);
        }
        glutTimerFunc(16, timerCallback, 0); // ~60 FPS
    }

public:
    // Callbacks públicos que podem ser configurados
    std::function<void()> onDisplay;
    std::function<void(int, int)> onReshape;
    std::function<void(int, bool)> onKeyPress;
    std::function<void(int, bool)> onSpecialKey;
    std::function<void(int, bool, int, int)> onMouseClick;
    std::function<void(int, int)> onMouseMove;
    std::function<void()> onIdle;
    std::function<void(int)> onTimer;

    Window(int w = 800, int h = 600, const std::string& t = "Ocean Engine")
        : width(w), height(h), title(t), fullscreen(false), windowHandle(0)
    {
        currentInstance = this;
    }

    ~Window() {
        if (currentInstance == this) {
            currentInstance = nullptr;
        }
    }

    // Inicializa a janela
    bool init(int* argc, char** argv) {
        glutInit(argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
        glutInitWindowSize(width, height);
        glutInitWindowPosition(100, 100);
        windowHandle = glutCreateWindow(title.c_str());

        if (windowHandle <= 0) {
            std::cerr << "Erro ao criar janela GLUT!" << std::endl;
            return false;
        }

        // Configura callbacks
        glutDisplayFunc(displayCallback);
        glutReshapeFunc(reshapeCallback);
        glutKeyboardFunc(keyboardCallback);
        glutKeyboardUpFunc(keyboardUpCallback);
        glutSpecialFunc(specialKeyCallback);
        glutSpecialUpFunc(specialKeyUpCallback);
        glutMouseFunc(mouseCallback);
        glutMotionFunc(motionCallback);
        glutPassiveMotionFunc(passiveMotionCallback);
        glutIdleFunc(idleCallback);
        glutTimerFunc(16, timerCallback, 0);

        // Configurações OpenGL básicas
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        
        glClearColor(0.0f, 0.0f, 0.1f, 1.0f);

        return true;
    }

    // Inicia o loop principal
    void run() {
        glutMainLoop();
    }

    // Getters
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getAspectRatio() const { 
        return height > 0 ? static_cast<float>(width) / height : 1.0f; 
    }

    // Setters
    void setTitle(const std::string& t) {
        title = t;
        glutSetWindowTitle(title.c_str());
    }

    void setSize(int w, int h) {
        width = w;
        height = h;
        glutReshapeWindow(w, h);
    }

    void toggleFullscreen() {
        fullscreen = !fullscreen;
        if (fullscreen) {
            glutFullScreen();
        } else {
            glutReshapeWindow(width, height);
            glutPositionWindow(100, 100);
        }
    }

    // Esconde/mostra cursor
    void hideCursor() {
        glutSetCursor(GLUT_CURSOR_NONE);
    }

    void showCursor() {
        glutSetCursor(GLUT_CURSOR_INHERIT);
    }

    // Centraliza o cursor (útil para FPS camera)
    void centerCursor() {
        glutWarpPointer(width / 2, height / 2);
    }

    // Solicita redesenho
    void requestRedraw() {
        glutPostRedisplay();
    }

    // Fecha a janela
    void close() {
        glutDestroyWindow(windowHandle);
    }
};

// Inicialização do ponteiro estático
Window* Window::currentInstance = nullptr;

} // namespace Ocean

#endif // WINDOW_HPP
