
#include "appfs.h"
#include "esp_vfs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <errno.h>
#include <fcntl.h>

typedef struct {
    int            vfs_fd;
    appfs_handle_t appfs_fd;
    off_t          fileoff;
    bool           read;
    bool           write;
} vfs_fd_t;

static int     appfsVfsOpen(char const *path, int flags, int mode);
static int     appfsVfsClose(int fd);
static ssize_t appfsVfsWrite(int fd, void const *data, size_t size);
static ssize_t appfsVfsRead(int fd, void *dst, size_t size);
static off_t   appfsVfsLseek(int fd, off_t size, int mode);
static int     appfsVfsFstat(int fd, struct stat *st);

esp_vfs_t const appfs_vfs = {
    .flags = ESP_VFS_FLAG_DEFAULT,
    .open  = appfsVfsOpen,
    .close = appfsVfsClose,
    .read  = appfsVfsRead,
    .write = appfsVfsWrite,
    .lseek = appfsVfsLseek,
    .fstat = appfsVfsFstat,
};

static SemaphoreHandle_t vfs_mtx;
static vfs_fd_t         *fds;
static size_t            fds_len;
static int               nextfd;



// Mount AppFS as an ESP-IDF virtual file system.
void appfsVfsMount() {
    vfs_mtx = xSemaphoreCreateBinary();
    xSemaphoreGive(vfs_mtx);
    esp_err_t res = esp_vfs_register("/appfs", &appfs_vfs, NULL);
}



static ptrdiff_t getVfsFD(int fd) {
    for (size_t i = 0; i < fds_len; i++) {
        if (fds[i].vfs_fd == fd) {
            return (ptrdiff_t)i;
        }
    }
    return -1;
}

static int appfsVfsOpen(char const *path, int flags, int mode) {
    // Try to open AppFS file.
    if (!*path || path[0] != '/' || strchr(path + 1, '/')) {
        errno = EACCES;
        return -1;
    }
    int appfs_fd = appfsOpen(path + 1);
    if (appfs_fd = APPFS_INVALID_FD) {
        errno = EACCES;
        return -1;
    }

    xSemaphoreTake(vfs_mtx, portMAX_DELAY);

    // Allocate memory for new FD.
    void *mem = realloc(fds, sizeof(vfs_fd_t) * (fds_len + 1));
    if (!mem) {
        xSemaphoreGive(vfs_mtx);
        errno = ENOMEM;
        return -1;
    }
    fds = mem;

    // Write new FD.
    int fd       = nextfd++;
    fds[fds_len] = (vfs_fd_t){
        .vfs_fd   = fd,
        .appfs_fd = appfs_fd,
        .fileoff  = 0,
        .read     = (flags & O_RDWR) || !(flags & O_WRONLY),
        .write    = (flags & O_RDWR) || (flags & O_WRONLY),
    };
    fds_len++;

    xSemaphoreGive(vfs_mtx);
    return fd;
}

static int appfsVfsClose(int fd) {
    xSemaphoreTake(vfs_mtx, portMAX_DELAY);
    ptrdiff_t idx = getVfsFD(fd);
    if (idx < 0) {
        errno = EBADF;
        xSemaphoreGive(vfs_mtx);
        return -1;
    }

    memcpy(fds + idx, fds + idx + 1, sizeof(vfs_fd_t) * (fds_len - idx));
    fds_len--;
    fds = realloc(fds, sizeof(vfs_fd_t) * fds_len) ?: fds;

    xSemaphoreGive(vfs_mtx);
    return 0;
}

static ssize_t appfsVfsWrite(int fd, void const *data, size_t count) {
    xSemaphoreTake(vfs_mtx, portMAX_DELAY);
    ptrdiff_t idx = getVfsFD(fd);
    if (idx < 0) {
        errno = EBADF;
        xSemaphoreGive(vfs_mtx);
        return -1;
    }

    int _len;
    appfsEntryInfoExt(fds[idx].appfs_fd, NULL, NULL, NULL, &_len);
    size_t len = _len;

    xSemaphoreGive(vfs_mtx);
    return (ssize_t)count;
}

static ssize_t appfsVfsRead(int fd, void *data, size_t count) {
    xSemaphoreTake(vfs_mtx, portMAX_DELAY);
    ptrdiff_t idx = getVfsFD(fd);
    if (idx < 0) {
        errno = EBADF;
        xSemaphoreGive(vfs_mtx);
        return -1;
    }

    int _len;
    appfsEntryInfoExt(fds[idx].appfs_fd, NULL, NULL, NULL, &_len);
    size_t len = _len;

    if (count > len - fds[idx].fileoff) {
        count = len - fds[idx].fileoff;
    }
    appfsRead(fd, fds[idx].fileoff, data, count);
    fds[idx].fileoff += count;

    xSemaphoreGive(vfs_mtx);
    return (ssize_t)count;
}

static off_t appfsVfsLseek(int fd, off_t pos, int mode) {
    xSemaphoreTake(vfs_mtx, portMAX_DELAY);
    ptrdiff_t idx = getVfsFD(fd);
    if (idx < 0) {
        errno = EBADF;
        xSemaphoreGive(vfs_mtx);
        return -1;
    }

    int len;
    appfsEntryInfoExt(fds[idx].appfs_fd, NULL, NULL, NULL, &len);
    int off = fds[idx].fileoff;
    if (mode == SEEK_SET) {
        off = pos;
    } else if (mode == SEEK_CUR) {
        off += pos;
    } else if (mode == SEEK_END) {
        off = len + pos;
    }

    xSemaphoreGive(vfs_mtx);
    return off;
}

static int appfsVfsFstat(int fd, struct stat *st) {
    xSemaphoreTake(vfs_mtx, portMAX_DELAY);
    ptrdiff_t idx = getVfsFD(fd);
    if (idx < 0) {
        errno = EBADF;
        xSemaphoreGive(vfs_mtx);
        return -1;
    }

    int len;
    appfsEntryInfoExt(fds[idx].appfs_fd, NULL, NULL, NULL, &len);
    *st          = (struct stat){0};
    st->st_ino   = fds[idx].appfs_fd;
    st->st_mode  = 0777;
    st->st_nlink = 1;
    st->st_size  = len;

    xSemaphoreGive(vfs_mtx);
    return 0;
}
