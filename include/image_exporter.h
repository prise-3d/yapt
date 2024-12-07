//
// Created by franck on 17/06/24.
//

#ifndef YAPT_IMAGE_EXPORTER_H
#define YAPT_IMAGE_EXPORTER_H

#include "yapt.h"
#include <string>
#include "image_data.h"
#include <memory>

class ImageExporter {
public:
    virtual ~ImageExporter() = default;

    virtual void write(std::string path) = 0;
};

class PNGImageExporter: public ImageExporter {
public:
    explicit PNGImageExporter(shared_ptr<ImageData> imageData): imageData(imageData) {}
    virtual void write(std::string fileName) override;

protected:
    shared_ptr<ImageData> imageData;
};

class EXRImageExporter: public ImageExporter {
public:
    explicit EXRImageExporter(shared_ptr<ImageData> imageData): imageData(imageData) {}
    explicit EXRImageExporter(shared_ptr<ImageData> imageData, size_t render_time): imageData(imageData), render_time(render_time) {}
    void write(std::string fileName) override;

protected:
    shared_ptr<ImageData> imageData;
    size_t render_time = 0;
};

#endif //YAPT_IMAGE_EXPORTER_H
