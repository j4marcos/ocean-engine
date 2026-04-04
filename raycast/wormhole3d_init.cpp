#include "wormhole3d_init.h"
#include "wormhole3d_globals.h"

#include <iostream>

#include <GL/gl.h>
#include <GL/glu.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void loadTexture(const char* filename) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

    if (data) {
        glGenTextures(1, &myTexture);
        glBindTexture(GL_TEXTURE_2D, myTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (nrChannels == 3) {
            gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
        } else if (nrChannels == 4) {
            gluBuild2DMipmaps(GL_TEXTURE_2D, 4, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        stbi_image_free(data);
    } else {
        std::cerr << "Falha ao carregar a textura: " << filename << std::endl;
    }
}

void initAppGl() {
    glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    loadTexture("raycast/textura.jpg");

    {
        int w, h, ch;
        auto tryLoad = [&](const char* path) -> GLuint {
            unsigned char* d = stbi_load(path, &w, &h, &ch, 0);
            if (!d) {
                return 0;
            }
            GLuint id;
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
            gluBuild2DMipmaps(GL_TEXTURE_2D, ch, w, h, fmt, GL_UNSIGNED_BYTE, d);
            stbi_image_free(d);
            return id;
        };
        gTexSky = tryLoad("raycast/textures/space.jpg");
        if (!gTexSky) {
            gTexSky = tryLoad("raycast/textures/stars.jpg");
        }
        if (!gTexSky) {
            gTexSky = tryLoad("raycast/stars.jpg");
        }
        if (!gTexSky) {
            std::cerr << "[AVISO] Sem textura de ceu (space.jpg / stars.jpg)\n";
        }
    }

    sphereQuadric = gluNewQuadric();
    gluQuadricTexture(sphereQuadric, GL_TRUE);
    gluQuadricNormals(sphereQuadric, GLU_SMOOTH);
}
