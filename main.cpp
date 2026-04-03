#include <iostream>
#include <atomic>

int main() {
    std::atomic<int> x{0};
    x.fetch_add(1, std::memory_order_relaxed);
    std::cout <<"atomics work: " << x.load() << "\n";
}