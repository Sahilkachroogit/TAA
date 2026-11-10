#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <vector>

static std::size_t g_alloc_bytes =0;
static inline void reset_alloc() noexcept{
    g_alloc_bytes=0;
}
static inline std::size_t snapshot_alloc() noexcept{
    return g_alloc_bytes;
}
static constexpr std::size_t HDR = alignof(std::max_align_t);
void* operator new(std::size_t n){
    void* raw = std::malloc(n+HDR);
    if(!raw) throw std::bad_alloc{};
    std::memcpy(raw,&n,sizeof(n));
    g_alloc_bytes +=n;
    return static_cast<char*>(raw)+HDR;
}
void* operator new[](std::size_t n){return operator new(n);}
void* operator new(std::size_t n,const std::nothrow_t&) noexcept{
    try{
        return ::operator new(n);
    }
    catch(...){
        return nullptr;
    }
}
void* operator new[](std::size_t n,const std::nothrow_t& nt)noexcept{
    return ::operator new(n,nt);
}
void operator delete(void* p) noexcept{
    if(!p) return;
    void* raw=static_cast<char*>(p)-HDR;
    std::size_t n=0;
    std::memcpy(&n,raw,sizeof(n));
    std::free(raw);
}
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { ::operator delete(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { ::operator delete(p); }
using Clock     = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

static inline TimePoint now() { return Clock::now(); }

static inline long long elapsed_us(TimePoint t0, TimePoint t1) {
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}
static constexpr uint64_t MOD = (1ULL << 61) - 1;
static uint64_t bin_pow_iter(uint64_t base,uint64_t exp,uint64_t mod)noexcept{
    uint64_t result=1;
    base %=mod;
    while(exp>0){
        if(exp&1u)result=(__uint128_t)result*base%mod;
        base=(__uint128_t)base*base%mod;
        exp>>=1;
    }
    return result;
}
static void bench_binpow(long long N,long long& out_us,std::size_t& out_bytes){
    volatile uint64_t sink=bin_pow_iter(2,static_cast<uint64_t>(N),MOD);
    (void)sink;
    reset_alloc();
    auto t0=now();
    for(long long i=1;i<=N;++i){
        sink^=bin_pow_iter(static_cast<uint64_t>(i & 0xff)+2,static_cast<uint64_t>(i),MOD);

    }
    auto t1=now();
    out_us=elapsed_us(t0,t1);
    out_bytes=snapshot_alloc();
}
static void bench_vector(long long N, long long& out_us, std::size_t& out_bytes) {
    // Warm-up
    {
        std::vector<int> v;
        v.reserve(1);
        for (long long i = 0; i < std::min(N, 16LL); ++i) v.push_back(static_cast<int>(i));
    }

    reset_alloc();
    auto t0 = now();
    {
        std::vector<int> v;      // no reserve — force natural realloc chain
        v.reserve(0);
        for (long long i = 0; i < N; ++i) {
            v.push_back(static_cast<int>(i));
        }
        volatile int last = v.empty() ? 0 : v.back();
        (void)last;
    } // destructor frees — not tracked (delete doesn't subtract)
    auto t1 = now();

    out_us    = elapsed_us(t0, t1);
    out_bytes = snapshot_alloc();
}
struct Node {
    long long value;
    Node* next;
    explicit Node(long long v) noexcept : value(v), next(nullptr) {}
};
static void bench_linked_list(long long N, long long& out_us, std::size_t& out_bytes) {
    // Warm-up: build and destroy a tiny list
    {
        Node* head = nullptr;
        for (int i = 0; i < 8; ++i) {
            Node* n = new Node(i);
            n->next = head;
            head = n;
        }
        while (head) { Node* tmp = head->next; delete head; head = tmp; }
    }

    reset_alloc();
    auto t0 = now();

    // --- Build ---
    Node* head = nullptr;
    Node* tail = nullptr;
    for (long long i = 0; i < N; ++i) {
        Node* n = new Node(i);
        if (!tail) { head = tail = n; }
        else       { tail->next = n; tail = n; }
    }

    // --- Traverse ---
    volatile long long acc = 0;
    for (Node* cur = head; cur; cur = cur->next) acc += cur->value;
    (void)acc;

    // --- Free ---
    while (head) {
        Node* tmp = head->next;
        delete head;
        head = tmp;
    }
    auto t1 = now();

    out_us    = elapsed_us(t0, t1);
    out_bytes = snapshot_alloc();
}
static std::vector<long long> build_sizes() {
    std::vector<long long> sizes;
    const double mult = 1.5;
    double v = 10.0;
    const long long cap = 1'000'000LL;
    while (static_cast<long long>(v) <= cap) {
        sizes.push_back(static_cast<long long>(v));
        v *= mult;
        if (v < 10.0) v = 10.0;   // guard against fp underflow
    }
    // Always include the exact cap
    if (sizes.empty() || sizes.back() != cap) sizes.push_back(cap);
    return sizes;
}
int main() {
    std::cout << "algorithm,input_size,execution_time_us,memory_bytes\n";

    const auto sizes = build_sizes();

    for (long long N : sizes) {
        long long us = 0;
        std::size_t mb = 0;

        bench_binpow(N, us, mb);
        std::cout << "binary_exponentiation," << N << ',' << us << ',' << mb << '\n';

        bench_vector(N, us, mb);
        std::cout << "vector_reallocation," << N << ',' << us << ',' << mb << '\n';

        bench_linked_list(N, us, mb);
        std::cout << "linked_list_traversal," << N << ',' << us << ',' << mb << '\n';
    }
    return 0;
}