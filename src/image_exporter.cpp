//
// Created by franck on 17/06/24.
//

#include "image_exporter.h"
#include <png.h>
#include <memory>
#include <iostream>
#include <vector>
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <ImfHeader.h>
#include <ImfIntAttribute.h>

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
    std::vector<uint8_t> png_data(imageData->data.size());

    for (int i = 0 ; i < imageData->data.size() ; ++i) {
        double value = imageData->data[i];
        value = linearToGamma(value);
        // Translate the [0,1] component values to the byte range [0,255].
        static const Interval intensity(0.000, 0.999);
        png_data[i] = static_cast<int>(256 * intensity.clamp(value));
    }
    if (write_png(fileName, png_data, imageData->width, imageData->height)) {
        std::clog << "Image successfully written to " << fileName << std::endl;
    } else {
        std::clog << "Error while writing to " << fileName << std::endl;
    }
}

void EXRImageExporter::write(std::string fileName) {

    const int width = imageData->width;
    const int height = imageData->height;

    Imf::Array2D<Imf::Rgba> pixels(width, height);
    for (int y = 0; y<height; y++) {
        for (int x = 0; x<width; x++) {
            const double r = imageData->data[3*(y * width + x)];
            const double g = imageData->data[3*(y * width + x) + 1];
            const double b = imageData->data[3*(y * width + x) + 2];
            pixels[y][x] = Imf::Rgba(r, g, b);
        }
    }

    try {
        const Imath_3_1::Box2i dataWindow(Imath_3_1::V2i(0, 0), Imath_3_1::V2i(width - 1, height - 1));
        const Imath_3_1::Box2i displayWindow(Imath_3_1::V2i(0, 0), Imath_3_1::V2i(width - 1, height - 1));

        Imf_3_2::Header header(displayWindow, dataWindow);

        header.insert("render_time", Imf_3_2::IntAttribute(render_time));

        Imf::RgbaOutputFile file (fileName.c_str(), header);
        file.setFrameBuffer (&pixels[0][0], 1, width);
        file.writePixels (height);
    } catch (const std::exception &e) {
        std::cerr << "error writing image file " <<  fileName << std::endl;
    }
}
