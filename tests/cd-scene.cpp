#include "scene.hpp"
#include <iostream>
using namespace rhine;
static void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
static unsigned visible(const SceneFrame& frame,int album){
    unsigned count=0;
    for(size_t i=0;i<frame.instances.size();++i){const auto& m=frame.instances[i].world;if(frame.albums[i]==album&&m._11*m._11+m._12*m._12+m._13*m._13>.5f)++count;}
    return count;
}
int main(){try{
    unsigned tested=0;
    for(int drives=1;drives<=3;++drives){
        json albums=json::array(),files=json::array();
        for(int i=0;i<drives;++i){albums.push_back({{"id","disc-"+std::to_string(i)},{"cd",{{"drive",std::string(1,char('F'+i))+":"}}}});files.push_back(i);}
        json library={{"albums",albums},{"columnFiles",json::array({files})}};
        Scene scene;scene.library(library);scene.skip();double now=0;
        auto sample=[&](bool reduced=false){now+=1./120;auto frame=scene.update(now,1./120,1600,900,reduced);for(int i=0;i<drives;++i)require(visible(frame,i)==1,"Optical drive lost its card or rendered duplicates");require(frame.instances.size()<=439,"Finite CDs expanded the instance pool");++tested;return frame;};
        auto first=sample(true);
        for(int step=0;step<120;++step){int current=step%drives;scene.select(current,now,scene.detail(),step%2?-1:1);scene.transport("disc-"+std::to_string(current),true);if(step%10==0)scene.open();if(step%10==5)scene.back();for(int frame=0;frame<3;++frame)sample();}
        for(int frame=0;frame<180;++frame)sample();
        if(drives==1){auto last=sample(true);require(std::abs(first.selected.x-last.selected.x)<.001f&&std::abs(first.selected.z-last.selected.z)<.001f,"Single-drive navigation scrolled through phantom CD rows");}
        // A media refresh may reorder drive albums; selection must retain its
        // ID and the card formerly occupying its row must stay available.
        std::reverse(albums.begin(),albums.end());scene.library({{"albums",albums},{"columnFiles",json::array({files})}});scene.select(scene.selected(),now,scene.detail());for(int frame=0;frame<180;++frame)sample();
        scene.library({{"albums",json::array()},{"columnFiles",json::array()}});auto empty=scene.update(now+.02,.02,1600,900,true);for(const auto& instance:empty.instances){const auto& m=instance.world;require(m._11*m._11+m._12*m._12+m._13*m._13<.001f,"Ejected media left a visible glass card");}
    }
    // Adding a drive must not shrink or replace the ordinary cyclic shelf.
    Scene mixed;mixed.library({{"albums",json::array({{{"id","music"}},{{"id","disc"},{"cd",{{"drive","V:"}}}}})},{"columnFiles",json::array({json::array({0}),json::array({1})})}});mixed.skip();auto frame=mixed.update(0,.016,1600,900,true);require(visible(frame,0)>100&&visible(frame,1)==1,"Finite drive handling changed the normal album array");
    std::cout<<json({{"ok",true},{"checkedFrames",tested},{"driveCounts",json::array({1,2,3})},{"ordinaryShelfPreserved",true}}).dump(2)<<"\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}}
