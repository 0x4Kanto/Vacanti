#include <ncurses.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <ctype.h>
#include <stdio.h>

#define MAX_FILES 1024
#define PATH_MAX_LEN 4096

char *files[MAX_FILES];
int file_count = 0;

// checks image extension
int is_image(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    return (strcasecmp(ext, ".png") == 0 ||
            strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".bmp") == 0);
}

// memory management
void free_files() {
    for (int i = 0; i < file_count; i++) {
        free(files[i]);
        files[i] = NULL;
    }
    file_count = 0;
}

// loads dir
void load_dir(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        perror("opendir");
        return;
    }

    struct dirent *dir;
    free_files();
    file_count = 0;

    while ((dir = readdir(d)) != NULL && file_count < MAX_FILES) {

        // hide dotfiles
        if (dir->d_name[0] == '.')
            continue;

        char fullpath[PATH_MAX_LEN * 2];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dir->d_name);

        struct stat st;
        if (stat(fullpath, &st) == -1)
            continue;

        // always show directories
        if (S_ISDIR(st.st_mode)) {
            char name[PATH_MAX_LEN];
            snprintf(name, sizeof(name), "%s/", dir->d_name);

            files[file_count] = strdup(name);
            if (!files[file_count]) {
                perror("strdup");
                break;
            }

            file_count++;
            continue;
        }

        // only show supported image files
        if (!is_image(dir->d_name))
            continue;

        files[file_count] = strdup(dir->d_name);
        if (!files[file_count]) {
            perror("strdup");
            break;
        }

        file_count++;
    }

    closedir(d);
}

// render SDL
void view_image(const char *filename) {
    endwin(); // stop ncurses and render SDL

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        goto restore_ncurses;
    }

    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) &
          (IMG_INIT_JPG | IMG_INIT_PNG))) {
        fprintf(stderr, "IMG_Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        goto restore_ncurses;
    }

    SDL_Surface *image = IMG_Load(filename);
    if (!image) {
        fprintf(stderr, "IMG_Load Error: %s\n", IMG_GetError());
        goto cleanup;
    }

    SDL_Window *window = SDL_CreateWindow(
        filename,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        image->w,
        image->h,
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, image);
    SDL_FreeSurface(image);

    if (!window || !renderer || !texture) {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        goto cleanup;
    }

    int running = 1;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN)
                running = 0;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

cleanup:
    IMG_Quit();
    SDL_Quit();

restore_ncurses:
    initscr();
    noecho();
    keypad(stdscr, TRUE);
}

// helper to remove trailing slash from directory names
void strip_trailing_slash(char *dst, const char *src) {
    strcpy(dst, src);

    size_t len = strlen(dst);
    if (len > 0 && dst[len - 1] == '/')
        dst[len - 1] = '\0';
}

int main(int argc, char *argv[]) {
    char cwd[PATH_MAX_LEN];

    // use supplied directory if one was given
    if (argc > 1) {
        if (realpath(argv[1], cwd) == NULL) {
            perror("realpath");
            return 1;
        }

        struct stat st;
        if (stat(cwd, &st) == -1) {
            perror("stat");
            return 1;
        }

        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "%s is not a directory\n", cwd);
            return 1;
        }

        if (chdir(cwd) == -1) {
            perror("chdir");
            return 1;
        }
    } else {
        if (!getcwd(cwd, sizeof(cwd))) {
            perror("getcwd");
            return 1;
        }
    }

    initscr();
    noecho();
    keypad(stdscr, TRUE);

    int highlight = 0;
    int ch;

    load_dir(cwd);

    while (1) {
        clear();

        mvprintw(0, 0, "Dir: %s (q to quit)", cwd);

        for (int i = 0; i < file_count; i++) {
            if (i == highlight)
                attron(A_REVERSE);

            mvprintw(i + 1, 0, "%s", files[i]);

            if (i == highlight)
                attroff(A_REVERSE);
        }

        refresh();

        ch = getch();

        switch (ch) {
            case KEY_UP:
                if (file_count > 0)
                    highlight = (highlight - 1 + file_count) % file_count;
                break;

            case KEY_DOWN:
                if (file_count > 0)
                    highlight = (highlight + 1) % file_count;
                break;

            case 10: {
                if (file_count == 0)
                    break;

                char selected[PATH_MAX_LEN];
                strip_trailing_slash(selected, files[highlight]);

                char path[PATH_MAX_LEN * 2];
                snprintf(path, sizeof(path), "%s/%s", cwd, selected);

                struct stat st;
                if (stat(path, &st) == -1) {
                    perror("stat");
                    break;
                }

                if (S_ISDIR(st.st_mode)) {
                    if (chdir(path) == -1) {
                        perror("chdir");
                        break;
                    }

                    if (!getcwd(cwd, sizeof(cwd))) {
                        perror("getcwd");
                        break;
                    }

                    load_dir(cwd);
                    highlight = 0;
                }
                else if (is_image(selected)) {
                    view_image(path);
                }

                break;
            }

            case 'q':
                free_files();
                endwin();
                return 0;
        }
    }
}
