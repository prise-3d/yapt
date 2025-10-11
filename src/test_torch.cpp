#include <torch/torch.h>
#include <iostream>

int main() {
    std::cout << "Testing libtorch..." << std::endl;

    // Simple tensor
    torch::Tensor tensor = torch::rand({2, 3});
    std::cout << "Random tensor:\n" << tensor << std::endl;

    std::cout << "CUDA available: " << (torch::cuda::is_available() ? "Yes" : "No") << std::endl;

    // trivial Neural Network
    torch::nn::Linear linear(3, 1);

    torch::Tensor input = torch::randn({1, 3});
    torch::Tensor output = linear(input);

    std::cout << "Input:\n" << input << std::endl;
    std::cout << "Output:\n" << output << std::endl;

    return 0;
}