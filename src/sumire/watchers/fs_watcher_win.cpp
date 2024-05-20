#ifdef _WIN32

#include <sumire/watchers/fs_watcher_win.hpp>

#include <Windows.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <locale>
#include <codecvt>

namespace sumire::watchers {

    FsWatcherWin::FsWatcherWin(
        const std::string& dir, FsWatchListener* listener, bool recursive
    ) : watchDir{ dir },
        listener{ listener },
        recursive{ recursive },
        watchHandle{getWatchHandle()} 
    {}

    FsWatcherWin::~FsWatcherWin() {
        if (watching) {
            endWatch();
        }
    }

    void FsWatcherWin::watch() {
        std::cout << "[Sumire::FsWatcherWin] INFO: Beginning watch of dir " << watchDir << std::endl;
        watching = true;

        watcherThread = std::make_unique<std::thread>(& FsWatcherWin::asyncWatch, this);
    }
    
    void FsWatcherWin::asyncWatch() {
        auto start = std::chrono::steady_clock::now();

        while (watching) {
            DWORD bytesReturned = 0;
            BOOL res = ReadDirectoryChangesW(
                watchHandle.handle,
                watchHandle.buffer.data(),
                NOTIF_BUFFER_ALLOC_SIZE,
                recursive,
                FILE_NOTIFY_CHANGE_LAST_WRITE, // For now, only interested in modified files.
                &bytesReturned,
                NULL,
                NULL
            );

            // Handle watch callbacks
            if (res != 0) {
                FILE_NOTIFY_INFORMATION* info =
                    reinterpret_cast<FILE_NOTIFY_INFORMATION*>(watchHandle.buffer.data());

                do {
                    // Get notif data
                    FsWatchAction action = toFsWatchAction(info->Action);
                    std::wstring w_Filename{ info->FileName, info->FileNameLength / sizeof(wchar_t) };
                    std::string filename = toString(w_Filename);

                    // Handle action
                    listener->handleFileAction(
                        action,
                        watchDir,
                        filename
                    );

                    // Next notif
                    info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                        reinterpret_cast<BYTE*>(info) + info->NextEntryOffset
                    );

                } while (info->NextEntryOffset > 0);
            }

            // TODO: Deal with double modification notifications
            auto now = std::chrono::steady_clock::now();
            size_t elapsed = static_cast<size_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());
        }
    }

    void FsWatcherWin::endWatch() {
        watching = false;

        std::cout << "[Sumire::FsWatcherWin] INFO: Ending watch of dir " << watchDir << std::endl;
        CancelIoEx(watchHandle.handle, NULL);
        watcherThread->join();
        std::cout << "[Sumire::FsWatcherWin] INFO: Watch thread finished for dir " << watchDir << std::endl;

        watcherThread = nullptr;
    }

    FsWatcherWin::DirWatchHandle FsWatcherWin::getWatchHandle() {
        DirWatchHandle watchHandle{};
        watchHandle.handle = CreateFileA(
            watchDir.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

        return std::move(watchHandle);
    }

    FsWatchAction FsWatcherWin::toFsWatchAction(DWORD action) {
        switch (action) {
        case FILE_ACTION_ADDED:            return FsWatchAction::FS_ADD;
        case FILE_ACTION_MODIFIED:         return FsWatchAction::FS_MODIFIED;
        case FILE_ACTION_REMOVED:          return FsWatchAction::FS_DELETE;
        case FILE_ACTION_RENAMED_NEW_NAME: return FsWatchAction::FS_MOVED;
        case FILE_ACTION_RENAMED_OLD_NAME: return FsWatchAction::FS_MOVED;
        default:
            throw std::runtime_error("[Sumire::FsWatcherWin] Tried to convert invalid FILE_ACTION.");
        }

        // This should never run
        return FS_ADD;
    }

    std::string FsWatcherWin::toString(const std::wstring& wstr) {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;

        return converterX.to_bytes(wstr);
    }
}

#endif