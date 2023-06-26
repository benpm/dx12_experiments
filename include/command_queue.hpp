#pragma once

#include <common.hpp>
#include <queue>

class CommandQueue
{
public:
    ComPtr<ID3D12CommandQueue> queue;

    CommandQueue() = default;
    CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);

    ComPtr<ID3D12GraphicsCommandList2> GetCommandList();
    uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> cmdList);
    uint64_t Signal();
    bool IsFenceComplete(uint64_t fenceValue);
    void WaitForFenceValue(uint64_t fenceValue);
    void Flush();
    
private:
    ComPtr<ID3D12Device2> device;

    ComPtr<ID3D12Fence> fence;
    uint64_t fenceValue = 0;
    HANDLE fenceEvent;

    struct CmdAllocEntry
    {
        uint64_t fenceValue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
    };

    D3D12_COMMAND_LIST_TYPE type;
    std::queue<CmdAllocEntry> cmdAllocQueue;
    std::queue<ComPtr<ID3D12GraphicsCommandList2>> cmdListQueue;

    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
    ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator);
};