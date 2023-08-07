#pragma once

#include <common.hpp>
#include <mutex>
#include <unordered_set>

class DescriptorAllocatorPage;

/**
 * Represents a single descriptor allocation in a descriptor heap. It is a
 * move-only, self-freeing type. It wraps D3D12_CPU_DESCRIPTOR_HANDLE.
 */
class DescriptorAllocation {
   public:
    // Initializes as a null descriptor
    DescriptorAllocation();
    DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE handle,
        uint32_t numHandles,
        uint32_t descriptorSize,
        std::shared_ptr<DescriptorAllocatorPage> page);
    ~DescriptorAllocation();
    // Prevents copy and assignment, allows moving
    DescriptorAllocation(const DescriptorAllocation&) = delete;
    DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;
    DescriptorAllocation(DescriptorAllocation&& allocation) noexcept;
    DescriptorAllocation& operator=(DescriptorAllocation&& other) noexcept;
    // Checks if valid descriptor
    bool isNull() const;
    // Get descriptor handle at specified offset
    D3D12_CPU_DESCRIPTOR_HANDLE getDescHandle(uint32_t offset = 0u) const;
    // Get number of consecutive handles for this allocation
    uint32_t getNumHandles() const;

   private:
    // Base descriptor handle
    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
    // Number of descriptors in this allocation
    uint32_t numHandles = 0u;
    // Offset to the next descriptor
    uint32_t descriptorSize = 0u;
    // Pointer to the original page where this allocation came from
    std::shared_ptr<DescriptorAllocatorPage> page = nullptr;

    // Free the descriptor back to the heap it came from
    void free();

    friend class DescriptorAllocator;
};

/**
 * Allocates descriptors from a CPU-visible descriptor heap. Used when loading
 * new resources like textures. It manages all descriptors required to describe
 * resources that may be load and unloaded during runtime.
 * It uses a free list allocation scheme.
 */
class DescriptorAllocator {
   public:
    // Type of descriptor: CBV_SRV_UAV, SAMPLER, RTV, DSV
    const D3D12_DESCRIPTOR_HEAP_TYPE type;
    // Number of descriptors per heap allowed
    const uint32_t nDescPerHeap;

    DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t nDescPerHeap = 256u);
    virtual ~DescriptorAllocator();

    // Allocate given number of contiguous descriptors from a CPU visible
    // descriptor heap. nDescriptors cannot be more than nDescPerHeap
    DescriptorAllocation allocate(uint32_t nDescriptors = 1u);

    // When given frame is complete, release stale descriptors
    void releaseStaleDescriptors(uint64_t frameNumber);

   private:
    using DescriptorHeapPool =
        std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

    DescriptorHeapPool heapPool;
    // Indices of available heaps in pool
    std::unordered_set<size_t> availableHeaps;
    // Ensures thread safety when using public methods
    std::mutex allocMutex;

    // Create a new heap with nDescPerHeap descriptors
    std::shared_ptr<DescriptorAllocatorPage> createAllocatorPage();
};

class DescriptorAllocatorPage
    : public std::enable_shared_from_this<DescriptorAllocatorPage> {
   public:
    DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t nDescPerHeap);
};