module;

#include <d3d12.h>
#include <mutex>
#include <memory>
#include <vector>
#include <unordered_set>

export module descriptor_allocator;

export import common;

export class DescriptorAllocatorPage;

export class DescriptorAllocation
{
   public:
    DescriptorAllocation();
    DescriptorAllocation(
        D3D12_CPU_DESCRIPTOR_HANDLE handle,
        uint32_t numHandles,
        uint32_t descriptorSize,
        std::shared_ptr<DescriptorAllocatorPage> page
    );
    ~DescriptorAllocation();
    DescriptorAllocation(const DescriptorAllocation&) = delete;
    DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;
    DescriptorAllocation(DescriptorAllocation&& allocation) noexcept;
    DescriptorAllocation& operator=(DescriptorAllocation&& other) noexcept;
    bool isNull() const;
    D3D12_CPU_DESCRIPTOR_HANDLE getDescHandle(uint32_t offset = 0u) const;
    uint32_t getNumHandles() const;

   private:
    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
    uint32_t numHandles = 0u;
    uint32_t descriptorSize = 0u;
    std::shared_ptr<DescriptorAllocatorPage> page = nullptr;

    void free();

    friend class DescriptorAllocator;
};

export class DescriptorAllocator
{
   public:
    const D3D12_DESCRIPTOR_HEAP_TYPE type;
    const uint32_t nDescPerHeap;

    DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t nDescPerHeap = 256u);
    virtual ~DescriptorAllocator() = default;

    DescriptorAllocation allocate(uint32_t nDescriptors = 1u);
    void releaseStaleDescriptors(uint64_t frameNumber);

   private:
    using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

    DescriptorHeapPool heapPool;
    std::unordered_set<size_t> availableHeaps;
    std::mutex allocMutex;

    std::shared_ptr<DescriptorAllocatorPage> createAllocatorPage();
};

export class DescriptorAllocatorPage : public std::enable_shared_from_this<DescriptorAllocatorPage>
{
   public:
    DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t nDescPerHeap);
    void free(DescriptorAllocation&& allocation, uint64_t frameNumber);
};
