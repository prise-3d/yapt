/*
* This file is part of the YAPT distribution (https://github.com/prise-3d/yapt).
 * Copyright (c) 2025 PrISE-3D.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * --- ADDITIONAL PERMISSION UNDER GNU GPL VERSION 3 SECTION 7 ---
 *
 * If you modify this Program, or any covered work, by linking or
 * combining it with the Intel Math Kernel Library (MKL) (or a modified
 * version of that library), containing parts covered by the terms of
 * the Intel Simplified Software License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 */

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