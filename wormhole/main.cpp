

#include "Engine.h"
#include "Wormhole.h"
#include "Ray.h"
#include <vector>

Engine engine;

int main() {
    Wormhole wh(vec3(-3e10, 0.0f, 0.0f), vec3(4e10, 2e10, 0.0f), 8.54e36);
    vector<Ray> rays;
    
    // Create some rays aiming at mouth A
    for (int i = 0; i < 100; i++) {
    float height = i % 2 ? 1230000000 * i : -1230000000 * i;
        rays.push_back(Ray(vec2(-1e11, height), vec2(c, 0.0f), &wh));
    }
    // rays.push_back(Ray(vec2(-1e11, 0.0f), vec2(c, 0.0f), &wh));
    // rays.push_back(Ray(vec2(-1e11, -3.27606302719999999e10), vec2(c, 0.0f), &wh));

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    while(!glfwWindowShouldClose(engine.window)) {
        engine.run();
        
        wh.draw();

        if (!rays.empty()) {
            rays[0].draw(rays);
        }
        
        for (auto& ray : rays) {
            ray.step(1.0f);
        }

        glfwSwapBuffers(engine.window);
        glfwPollEvents();
    }
    
    return 0;
}