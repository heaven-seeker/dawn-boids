#ifndef C2_LOG_HPP
#define C2_LOG_HPP

#include <string_view>


namespace C2Core::Log {

void Info(std::string_view message);
void Info(const char* format, ...);

void Warning(std::string_view message);
void Warning(const char* format, ...);

void Error(std::string_view message);
void Error(const char* format, ...);

void ErrorDetailed(std::string_view message, const char* file, int line);
void Trace(const char* colorCode, const char* format, ...);

} // C2Core::Log

#endif // C2_LOG_HPP
