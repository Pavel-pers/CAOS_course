#pragma once

#include <cstddef>

static constexpr size_t kPageSize = 1 << 12;

bool IsValidPage(void* page);
