static rhine::json hapticProbe(){
    using namespace rhine;Controllers controllers;
    auto now=[](){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();};
    for(int i=0;i<10;++i){controllers.poll(now(),true,[](Action){});Sleep(50);}
    auto initial=controllers.status();if(initial.at("connected").empty())throw std::runtime_error("Haptic test requires a connected controller");
    json checks=json::array();auto require=[&](bool pass,const char* label){if(!pass)throw std::runtime_error(label);checks.push_back({{"name",label},{"pass",true}});};
    auto stopped=[](const json& state){const auto& results=state.at("vibration").at("lastResults");return !results.empty()&&std::all_of(results.begin(),results.end(),[](const json& r){return r.value("startResult",-1)==0&&r.value("stopResult",-1)==0;});};
    double started=now();require(controllers.pulse(started),"real SDK accepts stronger feedback");
    auto active=controllers.status();Sleep(90);controllers.feedback(now(),true);
    require(controllers.status().at("vibration").value("stops",1u)==0,"160 ms pulse does not end early");
    double ended=0;for(int i=0;i<30;++i){controllers.feedback(now(),true);if(stopped(controllers.status())){ended=now();break;}Sleep(4);}
    require(ended>0&&ended-started>=.16&&ended-started<.26,"feedback expires without input polling or rendering");
    auto normal=controllers.status();require(stopped(normal),"real SDK accepts motor reset");
    Sleep(400);double cancellationStarted=now();require(controllers.pulse(cancellationStarted),"real SDK accepts another short pulse");
    Sleep(55);double cancellationTime=now();controllers.feedback(cancellationTime,false);
    auto cancelled=controllers.status();require(stopped(cancelled)&&cancelled.at("vibration").value("pulses",0u)==2,"focus loss cancels feedback without rendering");
    require(initial.at("promptModel")=="ps4","physical validation uses the connected PS4 controller");
    for(const auto& result:active.at("vibration").at("lastResults"))if(result.value("name",std::string())=="DualShock 4")require(result.value("strongMotor",0u)==160&&result.value("weakMotor",0u)==104&&result.value("durationMs",0.)==160.,"PS4 amplitude increased and requested duration unchanged");
    return {{"ok",true},{"rendering",false},{"inputPollingDuringPulse",false},{"normalElapsedMs",(ended-started)*1000},{"cancelElapsedMs",(cancellationTime-cancellationStarted)*1000},{"checks",checks},{"normal",normal},{"cancelled",cancelled}};
}
