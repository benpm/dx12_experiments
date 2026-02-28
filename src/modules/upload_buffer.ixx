module;

#include <d3d12.h>
#include <wrl.h>
#include <deque>
#include <memory>

export module upload_buffer;

export import common;

export class UploadBuffer
{
   public:
    struct Allocation
    {
        void* cpu = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpu = 0;
    };

   private:
    class Page
    {
       public:
        Page(size_t sizeInBytes);
        ~Page();
        bool fits(size_t sizeInBytes, size_t alignment) const;
        Allocation allocate(size_t sizeInBytes, size_t alignment);
        void reset();

       private:
        ComPtr<ID3D12Resource> resource;
        void* cpuPtr = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpuPtr = 0;
        const size_t size;
        size_t offset = 0u;
    };

   public:
    const size_t pageSize;

    explicit UploadBuffer(size_t pageSize = 2_MB);
    Allocation allocate(size_t sizeInBytes, size_t alignment = 256u);
    void reset();

   private:
    using PagePool = std::deque<std::shared_ptr<Page>>;

    std::shared_ptr<Page> requestPage();
    PagePool pagePool;
    PagePool freePages;
    std::shared_ptr<Page> curPage = nullptr;
};
