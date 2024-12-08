#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>



#define CACHE_SIZE 10
#define BLOCK_SIZE 4096

typedef struct CachePage {
    int fd;               
    off_t offset;          
    char data[BLOCK_SIZE]; 
    int dirty;             
} CachePage;

CachePage cache[CACHE_SIZE]; 

// Функция для нахождения страницы в кэше по файловому дескриптору и смещению
CachePage *find_page(int fd, off_t offset) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].fd == fd && cache[i].offset == offset) {
            return &cache[i];
        }
    }
    return NULL;
}

// Функция для вытеснения случайной страницы
void evict_page() {
    int idx = rand() % CACHE_SIZE; // Выбираем случайную страницу
    if (cache[idx].dirty) {
        // Если страница была изменена, записываем данные на диск
        lseek(cache[idx].fd, cache[idx].offset, SEEK_SET);
        write(cache[idx].fd, cache[idx].data, BLOCK_SIZE);
        cache[idx].dirty = 0;
    }
    // Освобождаем страницу
    cache[idx].fd = -1;
}

// Функция для загрузки данных в кэш
int load_page(int fd, off_t offset) {
    CachePage *page = find_page(fd, offset);
    if (page != NULL) {
        return 0; // Страница уже в кэше
    }

    // Нужно загрузить страницу в кэш
    evict_page(); // Если кэш полон, вытесняем случайную страницу

    // Ищем первую свободную страницу в кэше
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].fd == -1) {
            page = &cache[i];
            break;
        }
    }

    // Читаем данные с диска
    lseek(fd, offset, SEEK_SET);
    read(fd, page->data, BLOCK_SIZE);
    page->fd = fd;
    page->offset = offset;
    page->dirty = 0;

    return 0;
}



int lab2_open(const char *path) {
    int fd = open(path, O_RDWR );
    if (fd < 0) {
        return -1;
    }
    return fd;
}

int lab2_close(int fd) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].fd == fd) {
            // Если данные были изменены, записываем их на диск
            if (cache[i].dirty) {
                lseek(cache[i].fd, cache[i].offset, SEEK_SET);
                write(cache[i].fd, cache[i].data, BLOCK_SIZE);
                cache[i].dirty = 0;
            }
            cache[i].fd = -1;
            return close(fd);
        }
    }
    return close(fd);
}

ssize_t lab2_read(int fd, void* buf, size_t count) {
    // Проверка на выравнивание буфера
    if ((uintptr_t)buf % BLOCK_SIZE != 0) {
        errno = EINVAL; 
        return -1;
    }

    // Выполняем чтение
    ssize_t bytes_read = read(fd, buf, count);
    if (bytes_read < 0) {
         return -1;
    }

    return bytes_read;
}


ssize_t lab2_write(int fd, const void* buf, size_t count) {
    // Проверка на выравнивание буфера
    if ((uintptr_t)buf % BLOCK_SIZE != 0) {
        errno = EINVAL; 

        return -1;
    }

    // Выполняем запись
    ssize_t bytes_written = write(fd, buf, count);
    if (bytes_written < 0) {
        return -1;
    }

    return bytes_written;
}


off_t lab2_lseek(int fd, off_t offset, int whence) {
    return lseek(fd, offset, whence);
}

int lab2_fsync(int fd) {
    // Синхронизируем все измененные страницы с диском
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].fd == fd && cache[i].dirty) {
            lseek(cache[i].fd, cache[i].offset, SEEK_SET);
            write(cache[i].fd, cache[i].data, BLOCK_SIZE);
            cache[i].dirty = 0;
        }
    }
    return fsync(fd);
}
