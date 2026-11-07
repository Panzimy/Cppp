
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <unordered_map>
#include <mutex>

static std::unordered_map<uint64_t, SIZE_T> allocations;
static std::mutex memory_mutex;

extern "C" {

__declspec(dllexport)
uint64_t sm_alloc(uint64_t size)
{
    if (size == 0 || size > 100000000)
        return 0;

    void* ptr = VirtualAlloc(
        nullptr,
        static_cast<SIZE_T>(size),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!ptr)
        return 0;

    uint64_t handle =
        static_cast<uint64_t>(
            reinterpret_cast<uintptr_t>(ptr)
        );

    {
        std::lock_guard<std::mutex> lock(memory_mutex);
        allocations[handle] = static_cast<SIZE_T>(size);
    }

    return handle;
}

__declspec(dllexport)
int sm_size(uint64_t handle, uint64_t* result)
{
    if (!result)
        return 0;

    std::lock_guard<std::mutex> lock(memory_mutex);

    auto it = allocations.find(handle);

    if (it == allocations.end())
        return 0;

    *result = static_cast<uint64_t>(it->second);
    return 1;
}

__declspec(dllexport)
int sm_read(
    uint64_t handle,
    uint64_t offset,
    uint8_t* result
)
{
    if (!result)
        return 0;

    std::lock_guard<std::mutex> lock(memory_mutex);

    auto it = allocations.find(handle);

    if (it == allocations.end())
        return 0;

    if (offset >= it->second)
        return 0;

    const uint8_t* ptr =
        reinterpret_cast<const uint8_t*>(
            static_cast<uintptr_t>(handle)
        );

    *result = ptr[offset];

    return 1;
}

__declspec(dllexport)
int sm_write(
    uint64_t handle,
    uint64_t offset,
    uint8_t value
)
{
    std::lock_guard<std::mutex> lock(memory_mutex);

    auto it = allocations.find(handle);

    if (it == allocations.end())
        return 0;

    if (offset >= it->second)
        return 0;

    uint8_t* ptr =
        reinterpret_cast<uint8_t*>(
            static_cast<uintptr_t>(handle)
        );

    ptr[offset] = value;

    return 1;
}

__declspec(dllexport)
int sm_free(uint64_t handle)
{
    std::lock_guard<std::mutex> lock(memory_mutex);

    auto it = allocations.find(handle);

    if (it == allocations.end())
        return 0;

    if (!VirtualFree(
        reinterpret_cast<void*>(
            static_cast<uintptr_t>(handle)
        ),
        0,
        MEM_RELEASE
    ))
        return 0;

    allocations.erase(it);

    return 1;
}

__declspec(dllexport)
uint64_t sm_allocated_count()
{
    std::lock_guard<std::mutex> lock(memory_mutex);
    return allocations.size();
}

}
