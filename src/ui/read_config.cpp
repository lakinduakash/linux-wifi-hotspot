//
// Created by lakinduakash on 14/04/19.
//



#include <iostream>
#include <fstream>
#include <algorithm>

#include "read_config.h"
#include "h_prop.h"

extern "C" {


int read_config_file() {
    // std::ifstream is RAII, i.e. no need to call close
    std::ifstream cFile(CONFIG_FILE);
    if (cFile.is_open()) {
        std::string line;
        while (getline(cFile, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                       line.end());
            if (line[0] == '#' || line.empty())
                continue;
            auto delimiterPos = line.find('=');
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            std::cout << name << " " << value << '\n';
        }

    } else {
        std::cerr << "Couldn't open config file for reading.\n";
    }

    return 0;
}
}
