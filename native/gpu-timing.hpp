#pragma once
#include "platform.hpp"
#include <deque>
#include <algorithm>
namespace rhine {
// GPU timestamps are read only after the frame fence has completed. Neither
// queries nor reporting introduce a CPU wait in the rendering hot path.
class GpuTiming {
    static constexpr UINT slots=3,queries=6;
    Com<ID3D12QueryHeap> heap_;
    Com<ID3D12Resource> readback_;
    UINT64 frequency_=0;
    uint64_t collected_=0;std::array<UINT64,queries> lastTicks_{};
    std::array<bool,slots> ready_{};
    std::deque<std::array<double,3>> samples_;
public:
    GpuTiming(ID3D12Device* device,ID3D12CommandQueue* queue){
        check(queue->GetTimestampFrequency(&frequency_),"GPU timestamp frequency");
        D3D12_QUERY_HEAP_DESC query{};query.Type=D3D12_QUERY_HEAP_TYPE_TIMESTAMP;query.Count=slots*queries;
        check(device->CreateQueryHeap(&query,IID_PPV_ARGS(&heap_)),"GPU timestamp heap");
        D3D12_HEAP_PROPERTIES memory{};memory.Type=D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=slots*queries*sizeof(UINT64);buffer.Height=1;buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        check(device->CreateCommittedResource(&memory,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback_)),"GPU timestamp readback");
    }
    void collect(unsigned frame){
        auto slot=frame%slots;if(!ready_[slot])return;
        const size_t offset=slot*queries*sizeof(UINT64);D3D12_RANGE range{offset,offset+queries*sizeof(UINT64)};void* mapped=nullptr;
        check(readback_->Map(0,&range,&mapped),"Read completed GPU timestamps");
        const auto* ticks=reinterpret_cast<const UINT64*>(static_cast<const unsigned char*>(mapped)+offset);
        std::copy_n(ticks,queries,lastTicks_.begin());++collected_;
        std::array<double,3> sample{};
        for(unsigned i=0;i<3;++i)sample[i]=ticks[i*2+1]>=ticks[i*2]?1000.0*double(ticks[i*2+1]-ticks[i*2])/double(frequency_):0;
        D3D12_RANGE written{};readback_->Unmap(0,&written);ready_[slot]=false;
        // Exclude initialization, SDK compilation and resize from steady-state
        // percentiles; retain a bounded rolling window of actual GPU samples.
        if(frame>90){samples_.push_back(sample);if(samples_.size()>600)samples_.pop_front();}
    }
    void mark(ID3D12GraphicsCommandList* list,unsigned frame,unsigned query){list->EndQuery(heap_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,frame%slots*queries+query);}
    void resolve(ID3D12GraphicsCommandList* list,unsigned frame){auto slot=frame%slots;list->ResolveQueryData(heap_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,slot*queries,queries,readback_.Get(),slot*queries*sizeof(UINT64));ready_[slot]=true;}
    json status()const{
        json output={{"source","GPU timestamp queries"},{"frequencyHz",frequency_},{"collectedFrames",collected_},{"lastRawTicks",lastTicks_},{"frameSpanIncludesQueueIdle",true},{"samples",samples_.size()},{"windowFrames",600},{"excludesStartupFrames",90}};
        for(unsigned i=0;i<3;++i){std::vector<double> values;double total=0;for(auto& frame:samples_){values.push_back(frame[i]);total+=frame[i];}std::sort(values.begin(),values.end());auto percentile=[&](double p){return values.empty()?0:values[std::min(values.size()-1,size_t(p*double(values.size()-1)))];};output[i==0?"frameMs":i==1?"rayTracingMs":"upscalerMs"]={{"mean",values.empty()?0:total/values.size()},{"p50",percentile(.5)},{"p95",percentile(.95)},{"p99",percentile(.99)}};}
        return output;
    }
};
}
