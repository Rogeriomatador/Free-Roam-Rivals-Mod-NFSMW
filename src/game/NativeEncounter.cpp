#include "NativeEncounter.h"
#include "NativeRivalPrototype.h"
#include "NativeVehicleFactory.h"
#include "GameBridge.h"
#include "ChallengeInputProbe.h"
#include "NativeRivalHud.h"
#include "../domain/LiveEncounter.h"
#include "../persistence/LiveRivalStore.h"
#include "../core/Log.h"
#include <windows.h>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <optional>
#include <sstream>
#include <cstring>

namespace frr::game {
namespace {
NativeEncounterConfig config{};
domain::LiveEncounter encounter{};
std::optional<persistence::LiveRivalStore> store;
persistence::LiveRivalRecord history{};
bool historyWritable=false;
std::uint64_t loadedProfile=0,lastResultAt=0,lastRaceLog=0,lastRejectionAt=0;
const char* lastRejection="none";
domain::OutrunOutcome lastResult=domain::OutrunOutcome::InProgress;
std::uintptr_t racePlayer=0,raceRoad=0,raceStatus=0;
domain::EncounterPoint lastPlayerForward{0,0,1};
template<typename... Args> void line(char (&target)[64],const char* format,Args... args) {
    std::snprintf(target,sizeof(target),format,args...);
}
NativeRivalHudView panel() {
    NativeRivalHudView v{};v.visible=true;
    line(v.lines[0],"RICO / GOLF GTI - FREE ROAM RIVALS");
    return v;
}
void record(const domain::LiveEncounterView& view) {
    if(!view.justFinished) return;
    lastResultAt=GetTickCount64();lastResult=view.outcome;
    const bool applied=persistence::recordLiveResult(history,view.outcome);
    bool saved=false;std::string error;
    if(applied&&historyWritable&&store) saved=store->save(history,error);
    std::ostringstream log;log<<"LiveEncounter result="<<domain::outrunOutcomeName(view.outcome)
        <<" reason="<<view.reason<<" modHistoryApplied="<<applied<<" modHistorySaved="<<saved<<" battles="<<history.battles
        <<" wins="<<history.wins<<" losses="<<history.losses<<" cashWrite=0 garageWrite=0";
    Log::instance().info(log.str());
    if(!error.empty()) Log::instance().warn("Live rival history save failed; original retained: "+error);
}
const char* rejectionText(const char* reason) {
    if(!std::strcmp(reason,"too_far")) return "APROXIME-SE DO RIVAL";
    if(!std::strcmp(reason,"not_behind_rival")) return "FIQUE ATRAS DO RIVAL";
    if(!std::strcmp(reason,"not_aligned")) return "SIGA NA MESMA DIRECAO E PISTA";
    if(!std::strcmp(reason,"different_elevation")) return "SIGA NO MESMO NIVEL DE ESTRADA";
    if(!std::strcmp(reason,"speed_difference")) return "APROXIME SUA VELOCIDADE DA DO RIVAL";
    if(!std::strcmp(reason,"too_slow")) return "AMBOS PRECISAM ESTAR EM MOVIMENTO";
    if(!std::strcmp(reason,"cooldown")) return "AGUARDE O INTERVALO ENTRE DESAFIOS";
    if(!std::strcmp(reason,"physics_not_advancing")) return "AGUARDANDO A FISICA VOLTAR A AVANCAR";
    if(!std::strcmp(reason,"disabled")) return "DESAFIOS DESATIVADOS";
    return "DESAFIO BLOQUEADO POR ESTADO INCERTO";
}
const char* resultText(domain::OutrunOutcome r) {
    switch(r) {
        case domain::OutrunOutcome::PlayerWon:return "VITORIA - VOCE ABRIU VANTAGEM";
        case domain::OutrunOutcome::RivalWon:return "DERROTA - RICO ABRIU VANTAGEM";
        case domain::OutrunOutcome::Draw:return "EMPATE";
        default:return "CORRIDA INTERROMPIDA";
    }
}
}
void configureNativeEncounter(const NativeEncounterConfig& value) {
    config=value;encounter=domain::LiveEncounter(value.tuning,value.challengeDistanceMeters);
    configureNativeRivalHud(value.hudEnabled);
    wchar_t executable[32768]{};
    const auto size=GetModuleFileNameW(nullptr,executable,32768);
    if(size&&size<32768) store.emplace(std::filesystem::path(executable).parent_path()/L"scripts"/L"FreeRoamRivals"/L"saves");
}
bool nativeEncounterRunning() {return encounter.running();}
void interruptNativeEncounter(const char* reason) {record(encounter.interrupt(reason));}
void showNativeRivalStatus(const char* message,const char* detail) {
    auto view=panel();line(view.lines[1],"%s",message);line(view.lines[2],"%s",detail);
    line(view.lines[4],"%s",nativeRivalRetirementHint());
    line(view.lines[5],"VITORIAS %llu / DERROTAS %llu",static_cast<unsigned long long>(history.wins),
        static_cast<unsigned long long>(history.losses));
    publishNativeRivalHud(view);
}
void tickNativeEncounter(const RuntimeSnapshot& world,const NativeOwnedSnapshot& rival,
    const domain::WorldMetricCalibration& metric,float dt,bool pressed) {
    if(!world.career.profileKeyAvailable||!world.career.profileKey) {interruptNativeEncounter("profile_unavailable");showNativeRivalStatus("AGUARDANDO PERFIL");return;}
    if(loadedProfile!=world.career.profileKey) {
        interruptNativeEncounter("profile_changed");loadedProfile=world.career.profileKey;
        lastResultAt=0;lastPlayerForward={0,0,1};
        history={loadedProfile};historyWritable=false;
        if(store) {
            const auto loaded=store->load(loadedProfile);
            if(loaded.ok) {history=loaded.record;historyWritable=config.persistHistory;}
            else Log::instance().warn("Live rival history load failed; original preserved: "+loaded.error);
        }
    }
    if(encounter.running()&&(racePlayer!=world.vehicles.playerIVehicle||raceRoad!=world.roadNetwork||raceStatus!=world.raceStatus))
        interruptNativeEncounter("world_identity_changed");
    const bool valid=world.mode==WorldProbeMode::FreeRoamCandidate&&!world.fadeScreen&&!world.inNIS&&
        !world.raceStatusLoading&&world.vehicles.independentPlayerCrossCheck&&world.playerMotion.available&&
        world.playerMotion.position.finite&&domain::validWorldMetricCalibration(metric)&&
        rival.owned&&rival.available&&rival.contextMatches&&rival.active&&!rival.loading&&!rival.destroyed&&rival.box.valid;
    if(!valid) {interruptNativeEncounter("world_or_vehicle_unavailable");showNativeRivalStatus("AGUARDANDO MUNDO E RIVAL");return;}
    const float unit=metric.worldUnitsPerMeter;
    const auto& p=world.playerMotion.position;
    const auto& velocity=world.playerMotion.linearVelocity;
    const float horizontal=std::sqrt(velocity.x*velocity.x+velocity.z*velocity.z);
    domain::LiveEncounterInput input{};
    input.player={{p.x/unit,p.y/unit,p.z/unit},lastPlayerForward,world.playerMotion.absoluteSpeed,velocity.finite};
    if(velocity.finite&&std::isfinite(horizontal)&&horizontal>0.01f) {
        input.player.forward={velocity.x/horizontal,0,velocity.z/horizontal};input.player.valid=true;
        lastPlayerForward=input.player.forward;
    }
    const auto& r=rival.box;
    const float direction=std::sqrt(r.forward.x*r.forward.x+r.forward.z*r.forward.z);
    input.rival={{r.center.x/unit,r.center.y/unit,r.center.z/unit},{0,0,0},rival.speed,false};
    if(std::isfinite(direction)&&direction>0.01f) {input.rival.forward={r.forward.x/direction,0,r.forward.z/direction};input.rival.valid=true;}
    input.worldSafe=valid;
    input.challengeSafe=rival.pursuit==domain::PursuitSafetyState::Clear&&rival.ownedPursuitClear&&ChallengeInputProbe::fallbackVirtualKey()!=0;
    input.deltaSeconds=dt;input.acceptPressed=pressed&&config.enabled&&ChallengeInputProbe::fallbackVirtualKey()!=0;
    domain::LiveEncounterView result{};
    if(config.enabled) result=encounter.tick(input);
    if(result.justStarted) {
        racePlayer=world.vehicles.playerIVehicle;raceRoad=world.roadNetwork;raceStatus=world.raceStatus;
        Log::instance().info("LiveEncounter started: native roaming opponent, observed leader trail, 300m-style outrun scoring; vanillaEventCreated=0 AIWrites=0");
    }
    if(pressed&&!result.justStarted&&!result.running) {
        lastRejectionAt=GetTickCount64();lastRejection=config.enabled?result.reason:"disabled";
        std::ostringstream rejected;rejected<<"LiveEncounter challenge rejected reason="<<lastRejection
            <<" dtSeconds="<<dt<<" playerSpeed="<<input.player.speed<<" rivalSpeed="<<input.rival.speed
            <<" playerPursuit="<<static_cast<int>(rival.pursuit)<<" rivalPursuitClear="<<rival.ownedPursuitClear;
        Log::instance().info(rejected.str());
    }
    if(result.justFinished) {
        std::ostringstream evidence;evidence<<"LiveEncounter terminal sample dtSeconds="<<dt
            <<" player="<<input.player.position.x<<","<<input.player.position.y<<","<<input.player.position.z
            <<" rival="<<input.rival.position.x<<","<<input.rival.position.y<<","<<input.rival.position.z
            <<" playerSpeed="<<input.player.speed<<" rivalSpeed="<<input.rival.speed;
        Log::instance().info(evidence.str());
    }
    record(result);
    const auto now=GetTickCount64();
    if(result.running&&now-lastRaceLog>=1000) {
        lastRaceLog=now;std::ostringstream log;
        log<<"LiveEncounter race leadMeters="<<result.signedLeadMeters<<" routeMatched="<<result.routeMatched
            <<" dtSeconds="<<dt<<" reason="<<result.reason<<" leader="<<domain::outrunLeaderName(result.leader)<<" hold="<<result.holdProgress<<" elapsed="<<result.elapsedSeconds;
        Log::instance().info(log.str());
    }
    auto view=panel();
    line(view.lines[5],"VITORIAS %llu / DERROTAS %llu",static_cast<unsigned long long>(history.wins),
        static_cast<unsigned long long>(history.losses));
    if(result.running) {
        line(view.lines[1],"OUTRUN - %s",result.leader==domain::OutrunLeader::Player?"VOCE LIDERA":"RICO LIDERA");
        line(view.lines[2],"VANTAGEM %.0f / %.0f METROS",std::abs(result.signedLeadMeters),config.tuning.winLeadMeters);
        line(view.lines[3],"%s",result.routeMatched?"MANTENHA A VANTAGEM PARA VENCER":"AGUARDANDO MESMA TRAJETORIA");
        line(view.lines[4],"TEMPO %.0f S - RIVAL CONTINUA APOS CORRIDA",result.elapsedSeconds);
        view.playerLeading=result.leader==domain::OutrunLeader::Player;view.holdProgress=result.holdProgress;
    } else {
        if(lastResultAt&&now-lastResultAt<8000) line(view.lines[1],"%s",resultText(lastResult));
        else line(view.lines[1],"RICO EXPLORANDO ROCKPORT / %.0f KM/H",rival.speed*3.6f);
        if(config.enabled&&result.available) line(view.lines[2],"[%c] DESAFIAR RICO",static_cast<char>(ChallengeInputProbe::fallbackVirtualKey()));
        else if(!config.enabled) line(view.lines[2],"DESAFIOS DESATIVADOS");
        else if(result.remainingCooldown>0) line(view.lines[2],"NOVO DESAFIO EM %.0f S",result.remainingCooldown);
        else if(!input.challengeSafe) line(view.lines[2],"DESAFIO BLOQUEADO: POLICIA OU ESTADO INCERTO");
        else line(view.lines[2],"SIGA ATRAS, ALINHADO, ATE %.0f METROS",config.challengeDistanceMeters);
        if(lastRejectionAt&&now-lastRejectionAt<3000) line(view.lines[2],"%s",rejectionText(lastRejection));
        line(view.lines[3],"F8 NAO REMOVE O RIVAL");
        line(view.lines[4],"%s",nativeRivalRetirementHint());
    }
    publishNativeRivalHud(view);
}
}
