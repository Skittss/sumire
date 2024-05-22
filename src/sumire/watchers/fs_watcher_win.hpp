#pragma once

#ifdef _WIN32

#include <sumire/watchers/fs_watch_listener.hpp>
#include <sumire/watchers/fs_watch_action.hpp>

// GLFW defines this macro and spams the console if redefined in Windows.h
#undef APIENTRY

// Windows includes, excluding some annoying macros.
#define NOMINMAX
#include <Windows.h>
#undef near
#undef far

#include <string>
#include <vector>
#include <array>
#include <thread>

namespace sumire::watchers {

    class FsWatcherWin {
    public:
        FsWatcherWin(
            const std::string& dir, FsWatchListener* listener, bool recursive);
        ~FsWatcherWin();

        void watch();
        void endWatch();

    private:
        constexpr static int NOTIF_BUFFER_ALLOC_SIZE = 4096;

        struct DirWatchHandle {
            HANDLE handle;
            std::array<BYTE, NOTIF_BUFFER_ALLOC_SIZE> buffer{};
        };

        DirWatchHandle getWatchHandle();
        static FsWatchAction toFsWatchAction(DWORD action);
        static std::string toString(const std::wstring& wstr);

        void asyncWatch();
        std::unique_ptr<std::thread> watcherThread;

        const std::string watchDir;
        DirWatchHandle watchHandle;
        FsWatchListener* listener;
        bool recursive;

        // TODO: atomic?
        bool watching = false;
    };

}

#endif