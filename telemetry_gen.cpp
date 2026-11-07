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
