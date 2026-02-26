#include <descriptor_allocator.hpp>
#include <window.hpp>

DescriptorAllocation::DescriptorAllocation()
{
    this->handle.ptr = 0u;
}

DescriptorAllocation::DescriptorAllocation(
    D3D12_CPU_DESCRIPTOR_HANDLE handle,
    uint32_t numHandles,
    uint32_t descriptorSize,
    std::shared_ptr<DescriptorAllocatorPage> page
)
    : handle(handle), numHandles(numHandles), descriptorSize(descriptorSize), page(page)
{
}

DescriptorAllocation::~DescriptorAllocation()
{
    this->free();
}

DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation) noexcept
    : handle(allocation.handle),
      numHandles(allocation.numHandles),
      descriptorSize(allocation.descriptorSize),
      page(std::move(allocation.page))
{
    allocation.handle.ptr = 0;
    allocation.numHandles = 0;
    allocation.descriptorSize = 0;
    allocation.page = nullptr;
}

DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& other) noexcept
{
    if (this != &other) {
        this->free();
        this->handle = other.handle;
        this->numHandles = other.numHandles;
        this->descriptorSize = other.descriptorSize;
        this->page = std::move(other.page);
        other.handle.ptr = 0;
        other.numHandles = 0;
        other.descriptorSize = 0;
        other.page = nullptr;
    }
    return *this;
}

bool DescriptorAllocation::isNull() const
{
    return this->handle.ptr == 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::getDescHandle(uint32_t offset) const
{
    assert(offset < this->numHandles && "Offset out of range");
    return { this->handle.ptr + (offset * this->descriptorSize) };
}

uint32_t DescriptorAllocation::getNumHandles() const
{
    return this->numHandles;
}

void DescriptorAllocation::free()
{
    if (!this->isNull() && this->page != nullptr) {
        this->page->free(std::move(*this), Application::nBuffers);
    }
}

void DescriptorAllocatorPage::free(DescriptorAllocation&& /*allocation*/, uint64_t /*frameNumber*/)
{
    // TODO: Implement free-list based deallocation
}