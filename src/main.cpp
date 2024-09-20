//
// Created by franck on 07/06/24.
//

#include "yapt.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
#include <chrono>
#include "texture.h"
#include "image_exporter.h"
#include "triangle.h"
#include <iomanip>
#include "demo.h"

#define PI_N 1000000000
#define PI_N_THREADS 20

void compute_pi() {

    int n_inside = 0;
    for (int i = 0 ; i < PI_N ; i++) {
        double x = randomDouble();
        double y = randomDouble();
        if (x * x + y * y < 1) n_inside++;
    }
    std::cout << 4 * double(n_inside) / double(PI_N) << std::endl;
}

#include <random>
#include <thread>
#include <vector>
#include <atomic>

struct alignas(64) AlignedInt {
    std::atomic<int> value{0};
};

void compute_some_pi(AlignedInt* count) {
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < PI_N / PI_N_THREADS; i++) {
        double x = dist(rng);
        double y = dist(rng);
        if (x * x + y * y < 1) count->value++;
    }
}

void compute_pi_several_threads() {
    AlignedInt n_inside[PI_N_THREADS];

    std::vector<std::thread> threads;
    for (int i = 0; i < PI_N_THREADS; ++i) {
        threads.emplace_back(compute_some_pi, &n_inside[i]);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    int inside = 0;
    for (int i = 0; i < PI_N_THREADS; i++) {
        inside += n_inside[i].value.load();
    }

    std::cout << 4 * double(inside) / double(PI_N) << std::endl;
}

int main(int argc, char* argv[]) {

    std::string path;

    if (argc > 1) {
        path = argv[1];
    } else {
        std::clog << " usage: yapt /path/to/destination/png" << std::endl;
        return 0;
    }

    auto start = std::chrono::high_resolution_clock::now();

    switch(1) {
        case 1: final(path); break;
        case 2: simple_light(path); break;
        case 3: compute_pi(); break;
        case 4: compute_pi_several_threads(); break;
        case 5: original(path); break;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
    std::clog << "Rendering duration: " << double(duration.count()) / 1000. << " s" << std::endl;
}