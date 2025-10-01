#pragma once

#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a, b)
#define UNIQUE_ID(v) CONCAT(v, __COUNTER__)
