//
// Created by franck on 17/01/25.
//

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
        randomSeed(100);
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
