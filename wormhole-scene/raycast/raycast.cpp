#include "wormhole3d_raycast.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_raycast_cpu.h"
#include "wormhole3d_raycast_gpu.h"
#include "scene_moving.h"

void raycastScene() {
    // sceneUpdateDynamicElements();
    if (gRaycastGpuReady && gUseGpuRaycast) {
        raycastSceneGpu();
    } else {
        raycastSceneCpu();
    }
}
