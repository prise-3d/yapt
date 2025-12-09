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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#ifdef FUNCTION_PARSING

#include "yapt.h"
#include <iostream>

#include "exprtk/exprtk.hpp"

class Function {
public:

    static shared_ptr<Function> from_file(std::string filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file " << filename << std::endl;
            return nullptr;
        }

        std::cerr << "Reading file " << filename << std::endl;
        std::string str_fn;

        std::getline(file, str_fn);
        return make_shared<Function>(str_fn);
    }

    explicit Function(std::string str_fn) {
        random_seed(100);
        std::cout << "compiling function " << str_fn << std::endl;
        symbol_table.add_variable("x", _x);
        symbol_table.add_variable("y", _y);
        symbol_table.add_function("RND", rnd_double);
        symbol_table.add_constants();

        expression.register_symbol_table(symbol_table);

        parser.compile(str_fn, expression);
    }

    double compute(const double x, const double y) {
        _x = x;
        _y = y;
        return expression.value();
    }

    double operator()(const double x, const double y) {
        _x = x;
        _y = y;
        return expression.value();
    }

private:
    double _x = 0.;
    double _y = 0.;
    exprtk::expression<double> expression;
    exprtk::symbol_table<double> symbol_table;
    exprtk::parser<double> parser;
};

#endif

#endif //FUNCTIONS_H
