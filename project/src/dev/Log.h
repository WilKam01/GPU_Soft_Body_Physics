#pragma once

#define LOG_WRITE(str) std::cout << str << "\n";
#define LOG_WARNING(str) std::cout << std::string("[Log Warning]: ") + str << "\n";
#define LOG_ERROR(str) throw std::runtime_error(std::string("[Log Error]: ") + str + "\n");