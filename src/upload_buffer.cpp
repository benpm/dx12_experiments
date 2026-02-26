#include <upload_buffer.hpp>
#include <window.hpp>

UploadBuffer::UploadBuffer(size_t pageSize) : pageSize(pageSize) {}

UploadBuffer::Allocation UploadBuffer::allocate(size_t sizeInBytes, size_t alignment)
{
    if (sizeInBytes > this->pageSize) {
        throw std::bad_alloc();
    }

    if (!this->curPage || !this->curPage->fits(sizeInBytes, alignment)) {
        this->curPage = this->requestPage();
    }

    return this->curPage->allocate(sizeInBytes, alignment);
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::requestPage()
{
    std::shared_ptr<Page> page = nullptr;

    if (!this->freePages.empty()) {
        page = this->freePages.front();
        this->freePages.pop_front();
    } else {
        page = std::make_shared<Page>(this->pageSize);
        this->pagePool.push_back(page);
    }

    return page;
}

void UploadBuffer::reset()
{
    this->curPage = nullptr;
    this->freePages = this->pagePool;
    for (auto& page : this->freePages) {
        page->reset();
    }
}

UploadBuffer::Page::Page(size_t sizeInBytes) : size(sizeInBytes)
{
    auto device = Window::get()->device;
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
    chkDX(device->CreateCommittedResource(
        &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&this->resource)
    ));
    this->gpuPtr = this->resource->GetGPUVirtualAddress();
    this->resource->SetName(L"UploadBuffer::Page");
    this->resource->Map(0, nullptr, &this->cpuPtr);
}

UploadBuffer::Page::~Page()
{
    this->resource->Unmap(0, nullptr);
    this->cpuPtr = nullptr;
    this->gpuPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool UploadBuffer::Page::fits(size_t sizeInBytes, size_t alignment) const
{
    const size_t alignedSize = align(sizeInBytes, alignment);
    const size_t alignedOffset = align(this->offset, alignment);
    return alignedOffset + alignedSize <= this->size;
}

UploadBuffer::Allocation UploadBuffer::Page::allocate(size_t sizeInBytes, size_t alignment)
{
    if (!this->fits(sizeInBytes, alignment)) {
        throw std::bad_alloc();
    }

    const size_t alignedSize = align(sizeInBytes, alignment);
    this->offset = align(this->offset, alignment);

    Allocation allocation;
    allocation.cpu = static_cast<uint8_t*>(this->cpuPtr) + this->offset;
    allocation.gpu = this->gpuPtr + this->offset;
    this->offset += alignedSize;
    return allocation;
}

void UploadBuffer::Page::reset()
{
    this->offset = 0u;
}
