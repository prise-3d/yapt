//
// Created by franck on 17/06/24.
//

#include "image_exporter.h"
#include <png.h>
#include <memory>
#include <iostream>
#include <vector>

using std::shared_ptr;

inline bool write_png(const std::string& file_name, const std::vector<uint8_t>& image_data, int width, int height) {
    FILE* fp = fopen(file_name.c_str(), "wb");
    if (!fp) {
        std::cerr << "Erreur d'ouverture du fichier" << std::endl;
        return false;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        std::cerr << "Erreur de création de la structure png" << std::endl;
        fclose(fp);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        std::cerr << "Erreur de création de la structure d'information png" << std::endl;
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        std::cerr << "Erreur lors de l'écriture de l'image PNG" << std::endl;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return false;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = const_cast<png_bytep>(&image_data[y * width * 3]);
    }

    png_set_rows(png_ptr, info_ptr, row_pointers.data());
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return true;
}

void PNGImageExporter::write(std::string fileName) {
    if (write_png(fileName, imageData->data, imageData->width, imageData->height)) {
        std::clog << "Image successfully written to " << fileName << std::endl;
    } else {
        std::clog << "Error while writing to " << fileName << std::endl;
    }
}