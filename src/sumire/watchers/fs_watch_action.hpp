#pragma once

namespace sumire::watchers {

    enum FsWatchAction {
        FS_ADD,
        FS_DELETE,
        FS_MODIFIED,
        FS_MOVED
    };

}