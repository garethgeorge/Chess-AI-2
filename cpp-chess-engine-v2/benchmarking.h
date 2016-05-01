#ifndef benchmarking_h
#define benchmarking_h

#include <vector>
#include <time.h>

namespace benchmarking {
    struct Benchmark {
        std::vector<clock_t> times;
        
        Benchmark() { };
        void push() {
            times.push_back(clock());
        }
        
        clock_t pop() {
            clock_t toreturn = clock() - times.back();
            times.pop_back();
            return toreturn;
        }
    };
}


#endif /* benchmarking_h */
