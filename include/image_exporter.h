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
    void write(std::string fileName) override;

protected:
    shared_ptr<ImageData> imageData;
};

#endif //YAPT_IMAGE_EXPORTER_H
