#pragma once
#include "platform.hpp"
#include <d3d11.h>
#include <array>
namespace rhine {
class GpuTiming11 {
    struct Slot {Com<ID3D11Query> disjoint,start,end;bool pending=false;};
    std::array<Slot,12> slots_;std::vector<double> history_;bool recording_=false;uint64_t dropped_=0;
public:
    explicit GpuTiming11(ID3D11Device* device){for(auto& slot:slots_){D3D11_QUERY_DESC description{D3D11_QUERY_TIMESTAMP_DISJOINT,0};check(device->CreateQuery(&description,&slot.disjoint),"GPU timestamp frequency");description.Query=D3D11_QUERY_TIMESTAMP;check(device->CreateQuery(&description,&slot.start),"GPU timestamp start");check(device->CreateQuery(&description,&slot.end),"GPU timestamp end");}}
    void begin(ID3D11DeviceContext* context,unsigned frame){
        for(auto& slot:slots_)if(slot.pending){D3D11_QUERY_DATA_TIMESTAMP_DISJOINT frequency{};UINT64 start=0,end=0;if(context->GetData(slot.disjoint.Get(),&frequency,sizeof(frequency),D3D11_ASYNC_GETDATA_DONOTFLUSH)!=S_OK||context->GetData(slot.start.Get(),&start,sizeof(start),D3D11_ASYNC_GETDATA_DONOTFLUSH)!=S_OK||context->GetData(slot.end.Get(),&end,sizeof(end),D3D11_ASYNC_GETDATA_DONOTFLUSH)!=S_OK)continue;slot.pending=false;if(!frequency.Disjoint&&frequency.Frequency&&end>=start){history_.push_back(double(end-start)*1000/frequency.Frequency);if(history_.size()>600)history_.erase(history_.begin());}}
        auto& current=slots_[frame%slots_.size()];recording_=!current.pending;if(!recording_){++dropped_;return;}context->Begin(current.disjoint.Get());context->End(current.start.Get());
    }
    void end(ID3D11DeviceContext* context,unsigned frame){if(!recording_)return;auto& current=slots_[frame%slots_.size()];context->End(current.end.Get());context->End(current.disjoint.Get());current.pending=true;recording_=false;}
    json status()const{auto sorted=history_;std::sort(sorted.begin(),sorted.end());auto percentile=[&](double p){return sorted.empty()?0:sorted[size_t((sorted.size()-1)*p)];};return {{"source","D3D11 hardware timestamp queries"},{"nonblocking",true},{"samples",sorted.size()},{"skippedSamples",dropped_},{"frameMs",{{"median",percentile(.5)},{"p90",percentile(.9)},{"p99",percentile(.99)}}}};}
};
}
