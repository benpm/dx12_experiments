#pragma once

#include <common.hpp>
#include <deque>

/**
 * Simple wrapper around resource created in upload heap. Employs linear
 * allocator that allocates memory in chunks. Used for allocating memory on the
 * CPU that will be copied to the GPU. The allocator reuses this memory instead
 * of making more allocations.
 *
 * NOTE: We assume that each instance is associated with only one single command
 * list / allocator.
 */
class UploadBuffer {
   public:
    struct Allocation {
        void* cpu = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpu = 0;
    };

   private:
    class Page {
       public:
        Page(size_t sizeInBytes);
        ~Page();
        // Checks if page can fit requested size and alignment block
        bool fits(size_t sizeInBytes, size_t alignment) const;
        // Allocates memory block in page, throwing std::bad_alloc if the
        // allocation size is too big
        Allocation allocate(size_t sizeInBytes, size_t alignment);
        // Reset the page for re-use
        void reset();

       private:
        ComPtr<ID3D12Resource> resource;
        void* cpuPtr = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpuPtr = 0;
        // Allocated page size in bytes
        const size_t size;
        // Current allocation offset in bytes
        size_t offset = 0u;
    };

   public:
    // Size of pages in bytes
    const size_t pageSize;

    explicit UploadBuffer(size_t pageSize = 2_MB);
    // Allocates memory block in upload heap of requested size and alignment
    Allocation allocate(size_t sizeInBytes, size_t alignment = 256u);
    // Release all allocated pages
    void reset();

   private:
    // Pool of memory pages
    using PagePool = std::deque<std::shared_ptr<Page>>;

    // Request a page, creating one or reusing one from the pool
    std::shared_ptr<Page> requestPage();
    // All pages created by this allocator
    PagePool pagePool;
    // Pages that are available for allocation
    PagePool freePages;
    // Current page being allocated from
    std::shared_ptr<Page> curPage = nullptr;
};