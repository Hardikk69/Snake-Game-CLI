// Minimal assertion helper: no framework is installed on the lab machine.
#pragma once
#include <cstdio>

static int failures = 0;
#define CHECK(cond, what) \
    do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, what); ++failures; } } while (0)
#define REPORT() (std::printf(failures ? "%d FAILED\n" : "all passed\n", failures), failures ? 1 : 0)
