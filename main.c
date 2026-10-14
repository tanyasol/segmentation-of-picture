#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "lodepng.h"


typedef struct DUS {
    int width;
    int height;
    int* parent;
    int* level;
} DUS;

unsigned char* load_png(const char* filename, unsigned int* width, unsigned int* height);
void write_png(const char* filename, const unsigned char* image, unsigned width, unsigned height);

int findset(DUS* elem, int key);
DUS* makeset(int width, int height);
void unionsets(DUS* elem, int key1, int key2);

void rgb_to_bw(const unsigned char* pict, unsigned char* bw, int width, int height);
void bw_to_rgb(const unsigned char* bw, unsigned char* pict, int bw_size);
void contrast(unsigned char* col, int bw_size);
void Gauss(unsigned char* col, unsigned char* blr_pic, int width, int height);
void sharp(unsigned char* pict, unsigned char* out, int width, int height);

void join(unsigned char* pict, DUS* elem, int width, int height, int dif);
int* counter_comp(unsigned char* pict, DUS* elem, int width, int height);
void del_noise(unsigned char* pict, DUS* elem, int* size, int min);
void make_comp(unsigned char* pict, int width, int height, int min, int dif);

void color(unsigned char* pict, unsigned char* finish, int width, int height);

int main(void)
{
    const char* input_file = "skull.png";
    unsigned int width, height;
    int size;
    int bw_size;

    unsigned char* picture = load_png(input_file, &width, &height);
    if (picture == NULL) {
        printf("Problem reading picture from the file %s. Error.\n", input_file);
        return -1;
    }

    size    = width * height * 4;
    bw_size = width * height;

    unsigned char* sh_pic   = (unsigned char*)malloc(width * height);
    unsigned char* bw_image = (unsigned char*)malloc(width * height);
    unsigned char* res      = (unsigned char*)malloc(width * height * 4);
    unsigned char* finish   = (unsigned char*)malloc(size * sizeof(unsigned char));
    unsigned char* res2     = (unsigned char*)malloc(bw_size * sizeof(unsigned char));

    rgb_to_bw(picture, bw_image, width, height);
    contrast(bw_image, bw_size);
    Gauss(bw_image, finish, width, height);

    make_comp(finish, width, height, 25, 5);
    color(finish, res, width, height);

    write_png("rgb.png", res, width, height);

    free(bw_image);
    free(sh_pic);
    free(finish);
    free(res);
    free(res2);
    free(picture);

    return 0;
}

unsigned char* load_png(const char* filename, unsigned int* width, unsigned int* height)
{
    unsigned char* image = NULL;
    int error = lodepng_decode32_file(&image, width, height, filename);
    if (error != 0) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }
    return image;
}

void write_png(const char* filename, const unsigned char* image, unsigned width, unsigned height)
{
    unsigned char* png;
    size_t pngsize;
    int error = lodepng_encode32(&png, &pngsize, image, width, height);
    if (error == 0) {
        lodepng_save_file(png, pngsize, filename);
    } else {
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }
    free(png);
}

int findset(DUS* elem, int key)
{
    if (elem->parent[key] != key) {
        elem->parent[key] = findset(elem, elem->parent[key]);
    }
    return elem->parent[key];
}

DUS* makeset(int width, int height)
{
    DUS* elem = (DUS*)malloc(sizeof(DUS));
    elem->width  = width;
    elem->height = height;

    elem->parent = (int*)malloc(width * height * sizeof(int));
    elem->level  = (int*)malloc(width * height * sizeof(int));

    for (int i = 0; i < width * height; i++) {
        elem->parent[i] = i;
        elem->level[i]  = 0;
    }
    return elem;
}

void unionsets(DUS* elem, int key1, int key2)
{
    int mem_key1 = findset(elem, key1);
    int mem_key2 = findset(elem, key2);
    if (mem_key1 == mem_key2) return;

    if (elem->level[mem_key1] < elem->level[mem_key2]) {
        elem->parent[mem_key1] = mem_key2;
    } else if (elem->level[mem_key1] > elem->level[mem_key2]) {
        elem->parent[mem_key2] = mem_key1;
    } else {
        elem->parent[mem_key2] = mem_key1;
        elem->level[mem_key1]++;
    }
}

void rgb_to_bw(const unsigned char* pict, unsigned char* bw, int width, int height)
{
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int k = (i * width + j) * 4;
            bw[i * width + j] = (unsigned char)(0.299 * pict[k]
                                              + 0.587 * pict[k + 1]
                                              + 0.114 * pict[k + 2]);
        }
    }
}

void bw_to_rgb(const unsigned char* bw, unsigned char* pict, int bw_size)
{
    for (int i = 0; i < bw_size; i++) {
        pict[i * 4]     = bw[i];
        pict[i * 4 + 1] = bw[i];
        pict[i * 4 + 2] = bw[i];
        pict[i * 4 + 3] = 255;
    }
}

void contrast(unsigned char* col, int bw_size)
{
    for (int i = 0; i < bw_size; i++) {
        if (col[i] < 55)  col[i] = 0;
        if (col[i] > 195) col[i] = 255;
    }
}

void Gauss(unsigned char* col, unsigned char* blr_pic, int width, int height)
{
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            blr_pic[width * i + j]  = 0.084 * col[width * i + j]
                                    + 0.084 * col[width * (i + 1) + j]
                                    + 0.084 * col[width * (i - 1) + j];
            blr_pic[width * i + j] += 0.084 * col[width * i + (j + 1)]
                                    + 0.084 * col[width * i + (j - 1)];
            blr_pic[width * i + j] += 0.063 * col[width * (i + 1) + (j + 1)]
                                    + 0.063 * col[width * (i + 1) + (j - 1)];
            blr_pic[width * i + j] += 0.063 * col[width * (i - 1) + (j + 1)]
                                    + 0.063 * col[width * (i - 1) + (j - 1)];
        }
    }
}

void sharp(unsigned char* pict, unsigned char* out, int width, int height)
{
    const float kernel[3][3] = {
        { -1.0f, -1.0f, -1.0f },
        { -1.0f,  9.0f, -1.0f },
        { -1.0f, -1.0f, -1.0f }
    };

    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            float count = 0.0f;
            for (int i1 = -1; i1 <= 1; i1++) {
                for (int j1 = -1; j1 <= 1; j1++) {
                    int pix = pict[(i + i1) * width + (j + j1)];
                    count += pix * kernel[j1 + 1][i1 + 1];
                }
            }
            int count_int = (int)count;
            if (count_int < 0)   count_int = 0;
            if (count_int > 255) count_int = 255;
            out[i * width + j] = (unsigned char)count_int;
        }
    }

    for (int i = 0; i < width; i++) {
        out[i] = pict[i];
        out[(height - 1) * width + i] = pict[(height - 1) * width + i];
    }
    for (int j = 0; j < height; j++) {
        out[j * width] = pict[j * width];
        out[j * width + width - 1] = pict[j * width + width - 1];
    }
}

void join(unsigned char* pict, DUS* elem, int width, int height, int dif)
{
    const int d[8][2] = {
        {-1,-1}, {-1,0}, {-1,1},
        { 0,-1},         { 0,1},
        { 1,-1}, { 1,0}, { 1,1}
    };

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int tmp_num = i * width + j;
            if (pict[tmp_num] == 0) continue;

            for (int k = 0; k < 8; k++) {
                int j1 = j + d[k][0];
                int i1 = i + d[k][1];

                if (j1 >= 0 && j1 < width && i1 >= 0 && i1 < height) {
                    int br = i1 * width + j1;
                    if (pict[br] > 0 && fabs(pict[tmp_num] - pict[br]) < dif) {
                        unionsets(elem, tmp_num, br);
                    }
                }
            }
        }
    }
}

int* counter_comp(unsigned char* pict, DUS* elem, int width, int height)
{
    int* size = (int*)calloc(width * height, sizeof(int));
    for (int i = 0; i < width * height; i++) {
        if (pict[i] > 0) {
            int par = findset(elem, i);
            size[par]++;
        }
    }
    return size;
}

void del_noise(unsigned char* pict, DUS* elem, int* size, int min)
{
    int n = elem->height * elem->width;
    for (int i = 0; i < n; i++) {
        if (pict[i] > 0) {
            int par = findset(elem, i);
            if (size[par] < min) pict[i] = 0;
        }
    }
}

void make_comp(unsigned char* pict, int width, int height, int min, int dif)
{
    DUS* elem = makeset(width, height);
    join(pict, elem, width, height, dif);

    int* size = counter_comp(pict, elem, width, height);
    del_noise(pict, elem, size, min);

    free(size);
    free(elem->parent);
    free(elem->level);
    free(elem);
}

void color(unsigned char* pict, unsigned char* finish, int width, int height)
{
    DUS* elem = makeset(width, height);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int key = i * width + j;
            if (pict[key] == 0) continue;
            if (j > 0 && pict[key - 1] > 0)     unionsets(elem, key, key - 1);
            if (i > 0 && pict[key - width] > 0) unionsets(elem, key, key - width);
        }
    }

    int comp_counter = 0;
    int* amount_comp = (int*)calloc(width * height, sizeof(int));

    for (int i = 0; i < width * height; i++) {
        if (pict[i] > 0) {
            int par = findset(elem, i);
            if (amount_comp[par] == 0) {
                amount_comp[par] = ++comp_counter;
            }
        }
    }

    const int col[][3] = {
        {220, 20, 60},   {255, 0, 0},     {199, 21, 133},
        {255, 105, 180}, {219, 112, 147}, {255, 20, 147},
        {178, 34, 34},   {255, 99, 71},   {255, 0, 127},
        {139, 0, 0},     {255, 160, 122}, {147, 112, 219}
    };
    const int col_counter = sizeof(col) / sizeof(col[0]);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int key = i * width + j;
            int out = (i * width + j) * 4;

            if (pict[key] > 0) {
                int par = findset(elem, key);
                int num_of_c = amount_comp[par] % col_counter;
                finish[out]     = col[num_of_c][0];
                finish[out + 1] = col[num_of_c][1];
                finish[out + 2] = col[num_of_c][2];
                finish[out + 3] = 255;
            } else {
                finish[out]     = 0;
                finish[out + 1] = 0;
                finish[out + 2] = 0;
                finish[out + 3] = 255;
            }
        }
    }

    free(amount_comp);
    free(elem->parent);
    free(elem->level);
    free(elem);
}
