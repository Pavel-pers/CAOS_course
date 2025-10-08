#pragma once

#include <cstddef>

namespace nostd {

void Print(const char* message);
void EPrint(const char* message);
void PrintTo(int fd, const char* message);
int WriteAll(int fd, const char* buf, size_t len);

}  // namespace nostd

[[deprecated("Use nostd::Print")]] void Print(const char* message);
[[deprecated("Use nostd::WriteAll")]] int WriteAll(int fd, const char* buf,
                                                   size_t len);
