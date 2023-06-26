#include <command_queue.hpp>

CommandQueue::CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    : device(device)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type =     type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
 
    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&this->queue)));
}

// Get or create a commandlist, using next available command allocator
ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList()
{
    ComPtr<ID3D12GraphicsCommandList2> commandList;
    ComPtr<ID3D12CommandAllocator> commandAllocator = this->CreateCommandAllocator();

    if (this->cmdListQueue.empty()) {
        commandList = this->CreateCommandList(commandAllocator);
    } else {
        commandList = this->cmdListQueue.front();
        ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
        this->cmdListQueue.pop();
    }

    // Assign allocator to command list
    ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

    return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> cmdList)
{
    ThrowIfFailed(cmdList->Close());

    // Retrieve command allocator from private data of command list
    ID3D12CommandAllocator* commandAllocator;
    UINT dataSize = sizeof(commandAllocator);
    ThrowIfFailed(cmdList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

    ID3D12CommandList* const commandLists[] = {
        cmdList.Get()
    };
    this->queue->ExecuteCommandLists(_countof(commandLists), commandLists);
    uint64_t fenceVal = this->Signal();

    // Push bach the allocator and list to their queues so they can be re-used
    this->cmdAllocQueue.emplace(CmdAllocEntry{ fenceVal, commandAllocator });
    this->cmdListQueue.push(cmdList);

    // Release ownership of the command allocator
    commandAllocator->Release();

    return fenceVal;
}

uint64_t CommandQueue::Signal()
{
    uint64_t fenceValueForSignal = ++fenceValue;
    ThrowIfFailed(this->queue->Signal(fence.Get(), fenceValueForSignal));

    return fenceValueForSignal;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
    return (this->fence->GetCompletedValue() < fenceValue);
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
    if (!this->IsFenceComplete(fenceValue))
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

void CommandQueue::Flush()
{
    uint64_t fenceValueForSignal = this->Signal();
    this->WaitForFenceValue(fenceValueForSignal);
}

// Command allocator can't be re-used unless associated cmd list's commands
//  have finished on the GPU (i.e. entry's fence value is greater than the current fence value)
ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
    ComPtr<ID3D12CommandAllocator> allocator;

    if (this->cmdAllocQueue.size() > 0 && this->IsFenceComplete(this->cmdAllocQueue.front().fenceValue)) {
        allocator = this->cmdAllocQueue.front().commandAllocator;
        ThrowIfFailed(allocator->Reset());
        this->cmdAllocQueue.pop();
    } else {
        ThrowIfFailed(device->CreateCommandAllocator(this->type, IID_PPV_ARGS(&allocator)));
    }

    return allocator;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator)
{
    ComPtr<ID3D12GraphicsCommandList2> commandList;
    ThrowIfFailed(device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    
    ThrowIfFailed(commandList->Close());

    return commandList;
}
