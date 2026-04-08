#include "wormhole3d_audio.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#if defined(__unix__) || defined(__APPLE__)
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

#if defined(__unix__) || defined(__APPLE__)

pid_t gMusicPid = -1;
bool gExitHookRegistered = false;

static bool fileReadable(const char* path) {
    return path != nullptr && access(path, R_OK) == 0;
}

static bool joinDirAssetMp3(char* out, size_t outLen, const char* dir) {
    if (dir == nullptr) {
        return false;
    }
    const size_t n = std::strlen(dir);
    static constexpr const char kSuffix[] = "/assets/ipanema.mp3";
    if (n + sizeof(kSuffix) > outLen) {
        return false;
    }
    std::memcpy(out, dir, n);
    std::memcpy(out + n, kSuffix, sizeof(kSuffix));
    return fileReadable(out);
}

static bool resolveMusicPath(char* out, size_t outLen, const char* argv0) {
    const char* relative[] = {
        "assets/ipanema.mp3",
        "./assets/ipanema.mp3",
        "wormhole-scene/assets/ipanema.mp3",
    };
    for (const char* rel : relative) {
        if (fileReadable(rel)) {
            if (realpath(rel, out) != nullptr) {
                return true;
            }
            std::snprintf(out, outLen, "%s", rel);
            return true;
        }
    }
#if defined(__linux__)
    {
        char self[PATH_MAX + 1];
        const ssize_t n = readlink("/proc/self/exe", self, PATH_MAX);
        if (n > 0 && n < static_cast<ssize_t>(sizeof(self))) {
            self[n] = '\0';
            char* const last = std::strrchr(self, '/');
            if (last != nullptr) {
                *last = '\0';
                if (joinDirAssetMp3(out, outLen, self)) {
                    return true;
                }
            }
        }
    }
#endif
    if (argv0 != nullptr && argv0[0] != '\0') {
        char buf[PATH_MAX + 1];
        std::snprintf(buf, sizeof(buf), "%s", argv0);
        char* const last = std::strrchr(buf, '/');
        if (last != nullptr) {
            *last = '\0';
            if (joinDirAssetMp3(out, outLen, buf)) {
                return true;
            }
        }
    }
    return false;
}

static void stopMusicOnExit() {
    if (gMusicPid > 0) {
        kill(gMusicPid, SIGTERM);
        (void)waitpid(gMusicPid, nullptr, 0);
        gMusicPid = -1;
    }
}

#endif

} // namespace

void startBackgroundMusic(const char* argv0) {
#if defined(__unix__) || defined(__APPLE__)
    char path[PATH_MAX + 1];
    if (!resolveMusicPath(path, sizeof(path), argv0)) {
        std::cerr << "Áudio: não encontrado assets/ipanema.mp3 (execute a partir de wormhole-scene ou instale no PATH: mpv ou ffplay).\n";
        return;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Áudio: fork falhou.\n";
        return;
    }
    if (pid == 0) {
        (void)setsid();
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        execlp("mpv", "mpv", "--no-terminal", "--no-video", "--really-quiet", "--loop-file=inf", path, static_cast<char*>(nullptr));
        execlp("ffplay", "ffplay", "-nodisp", "-loglevel", "quiet", "-loop", "0", path, static_cast<char*>(nullptr));
        _exit(127);
    }
    gMusicPid = pid;
    if (!gExitHookRegistered) {
        gExitHookRegistered = true;
        std::atexit(stopMusicOnExit);
    }
#else
    (void)argv0;
#endif
}
