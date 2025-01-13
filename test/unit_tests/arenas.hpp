#ifndef MCC_TEST_ARENAS_HPP
#define MCC_TEST_ARENAS_HPP

// Arenas for testing purpose

extern "C" {
#include <mcc/arena.h>
}

Arena& get_permanent_arena();
Arena get_scratch_arena();

#endif // MCC_TEST_ARENAS_HPP
