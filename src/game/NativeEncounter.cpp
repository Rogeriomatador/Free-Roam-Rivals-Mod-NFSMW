#include "NativeEncounter.h"
#include "NativeRivalPrototype.h"
#include "NativeVehicleFactory.h"
#include "GameBridge.h"
#include "ChallengeInputProbe.h"
#include "NativeRivalHud.h"
#include "VehicleSpatialProbe.h"
#include "../domain/LiveEncounter.h"
#include "../domain/StreetSprint.h"
#include "../domain/RivalLocator.h"
#include "../domain/RivalReaction.h"
#include "../domain/RivalMotionMonitor.h"
#include "../domain/DirectionBeacon.h"
#include "../domain/RaceFormationReadiness.h"
#include "../persistence/LiveRivalStore.h"
#include "../core/Log.h"
#include <windows.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <sstream>
#include <cstring>

namespace frr::game {
namespace {
NativeEncounterConfig config{};
domain::LiveEncounter encounter{};
domain::StreetSprint streetSprint{};
domain::RivalReactionMonitor reactionMonitor{};
domain::RivalMotionMonitor stationaryMonitor{};
std::optional<persistence::LiveRivalStore> store;
persistence::LiveRivalRecord history{};
bool historyWritable=false;
std::uint64_t loadedProfile=0,lastResultAt=0,lastRaceLog=0,lastRejectionAt=0,lastOvertakeAt=0;
const char* lastRejection="none";
domain::OutrunOutcome lastResult=domain::OutrunOutcome::InProgress;
domain::OutrunLeader lastOvertakeLeader=domain::OutrunLeader::Rival;
std::uintptr_t racePlayer=0,raceRoad=0,raceStatus=0;
domain::EncounterPoint lastPlayerForward{0,0,1};
enum class DuelPilotPhase { Idle, Slowing, Countdown, Releasing };
DuelPilotPhase duelPilotPhase=DuelPilotPhase::Idle;
float duelPilotElapsed=0,duelPilotCountdown=0;
std::uint64_t duelPilotCueUntil=0;
const char* duelPilotCue="none";
domain::EncounterCar duelStartPlayer{},duelStartRival{};
std::uint64_t lastSprintResultAt=0, modeToggleAt=0;
domain::OutrunOutcome lastSprintOutcome=domain::OutrunOutcome::InProgress;
void clearDuelPilot(const char* reason) {
    if(NativeVehicleFactory::duelStopHeld()) NativeVehicleFactory::releaseDuelStop();
    duelPilotPhase=DuelPilotPhase::Idle;
    duelPilotElapsed=0;duelPilotCountdown=0;
    duelPilotCue=reason;duelPilotCueUntil=GetTickCount64()+4000;
}
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
void recordSprint(const domain::StreetSprintView& result) {
    if(!result.justFinished) return;
    domain::LiveEncounterView recorded{};
    recorded.justFinished=true;
    recorded.outcome=result.outcome;
    recorded.reason=result.reason;
    record(recorded); // Reuses the existing per-profile persistent W/L/D/A store.
    lastSprintResultAt=GetTickCount64();
    lastSprintOutcome=result.outcome;
    Log::instance().info(std::string("StreetSprint finish scoredFromPhysicalSharedPlane=1 result=")+
        domain::outrunOutcomeName(result.outcome)+" reason="+result.reason+
        " playerMeters="+std::to_string(result.playerMeters)+
        " rivalMeters="+std::to_string(result.rivalMeters)+
        " timeSeconds="+std::to_string(result.elapsed));
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
    if(!std::strcmp(reason,"pursuit_or_input_unverified")) return "ESPERE TERMINAR A PERSEGUICAO";
    if(!std::strcmp(reason,"invalid_player_telemetry")) return "DIRIJA PARA CONFIRMAR MOVIMENTO";
    if(!std::strcmp(reason,"invalid_rival_telemetry")) return "AGUARDANDO MOVIMENTO DO RIVAL";
    if(!std::strcmp(reason,"none")) return "G: DESAFIAR RICO";
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
    streetSprint=domain::StreetSprint({value.streetSprintLengthMeters,16.0f,45.0f});
    lastSprintResultAt=0;
    reactionMonitor.reset();stationaryMonitor.reset();lastOvertakeAt=0;
    duelPilotPhase=DuelPilotPhase::Idle;duelPilotElapsed=0;duelPilotCountdown=0;
    configureNativeRivalHud(value.hudEnabled,value.radarPosition,value.radarScale);
    Log::instance().info("FRR overlay layout: viewportResponsive=1 radarPosition="+
        std::to_string(value.radarPosition)+" radarScale="+std::to_string(value.radarScale)+
        " nativeMinimapWrites=0 originalHUDPatches=0");
    wchar_t executable[32768]{};
    const auto size=GetModuleFileNameW(nullptr,executable,32768);
    if(size&&size<32768) store.emplace(std::filesystem::path(executable).parent_path()/L"scripts"/L"FreeRoamRivals"/L"saves");
}
bool nativeEncounterRunning() {
    return encounter.running() || streetSprint.running() ||
        duelPilotPhase!=DuelPilotPhase::Idle || NativeVehicleFactory::duelStopHeld();
}
void interruptNativeEncounter(const char* reason) {
    if(streetSprint.running()) recordSprint(streetSprint.abort(reason));
    if(duelPilotPhase!=DuelPilotPhase::Idle || NativeVehicleFactory::duelStopHeld())
        clearDuelPilot("INTERROMPIDO - RESTAURANDO CONTROLE");
    record(encounter.interrupt(reason));reactionMonitor.reset();stationaryMonitor.reset();lastOvertakeAt=0;
}
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
        lastResultAt=0;lastSprintResultAt=0;lastOvertakeAt=0;lastPlayerForward={0,0,1};reactionMonitor.reset();stationaryMonitor.reset();
        history={loadedProfile};historyWritable=false;
        if(store) {
            const auto loaded=store->load(loadedProfile);
            if(loaded.ok) {history=loaded.record;historyWritable=config.persistHistory;}
            else Log::instance().warn("Live rival history load failed; original preserved: "+loaded.error);
        }
    }
    if(ChallengeInputProbe::consumeModeToggle()) {
        // Do not switch into legacy Outrun while native ForceStop is still
        // owned: retry releasing the player first, without losing the lease.
        if(duelPilotPhase!=DuelPilotPhase::Idle || NativeVehicleFactory::duelStopHeld())
            clearDuelPilot("MODO PENDENTE - LIBERANDO VEICULOS");
        if(NativeVehicleFactory::duelStopHeld()) {
            Log::instance().warn("Challenge mode toggle postponed: native ForceStop lease still held");
        } else {
        if(streetSprint.running()) recordSprint(streetSprint.abort("mode_changed"));
        if(encounter.running()) record(encounter.interrupt("mode_changed"));
        config.nativeStreetDuelPilotEnabled=!config.nativeStreetDuelPilotEnabled;
        modeToggleAt=GetTickCount64();
        duelPilotCue=config.nativeStreetDuelPilotEnabled?
            "MODO SPRINT RETO - CORRIDA COM CHEGADA":
            "MODO OUTRUN - PERSEGUICAO ANTIGA";
        duelPilotCueUntil=modeToggleAt+5000;
        Log::instance().info(std::string("Challenge mode changed via Ctrl+Shift+G: ")+
            (config.nativeStreetDuelPilotEnabled?
             "short_physical_sprint_opt_in=1":"legacy_outrun=1")+
            " INI_and_saves_unchanged=1");
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
    input.deltaSeconds=dt;
    input.acceptPressed=pressed&&config.enabled&&!config.nativeStreetDuelPilotEnabled&&ChallengeInputProbe::fallbackVirtualKey()!=0;
    domain::LiveEncounterView result{};
    if(config.enabled) result=encounter.tick(input);
    if(result.justStarted) {
        racePlayer=world.vehicles.playerIVehicle;raceRoad=world.roadNetwork;raceStatus=world.raceStatus;
        Log::instance().info("LiveEncounter started: mode=Outrun_only observed leader trail, 300m separation; sideBySideStart=0 jointCountdown=0 rivalBrakeWrites=0 playerControlWrites=0 vanillaEventCreated=0");
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
    // Opt-in native two-car hold+release followed by a REAL shared physical
    // straight-line finish. Not the vanilla stock Sprint event or AI route.
    const bool pilotPressed=pressed && !streetSprint.running();
    if(pressed && streetSprint.running()) {
        recordSprint(streetSprint.abort("player_cancelled"));
    }
    // Real, opt-in ForceStop pilot. Starts only after verified native release:
    // no auto-steering, player input lock, native horn, finish or scoring.
    if(config.nativeStreetDuelPilotEnabled) {
        if(NativeVehicleFactory::duelStopHeld() && duelPilotPhase==DuelPilotPhase::Idle)
            NativeVehicleFactory::releaseDuelStop();
        bool geometry=false;
        const auto fleet=VehicleSpatialProbe::sample();
        if(fleet.registryComplete && world.vehicles.playerIVehicle) {
            for(const auto& box:fleet.boxes) {
                if(!box.valid || box.identity!=world.vehicles.playerIVehicle) continue;
                if(std::hypot(box.center.x-p.x,box.center.z-p.z)>5.0f*unit ||
                   std::abs(box.center.y-p.y)>5.0f*unit) continue;
                auto physicalPlayer=input.player;
                physicalPlayer.forward={box.forward.x,box.forward.y,box.forward.z};
                physicalPlayer.valid=true;
                const auto formation=domain::checkRaceFormation(physicalPlayer,input.rival,valid&&input.challengeSafe);
                geometry=formation.geometryAligned;
                if(geometry) {
                    duelStartPlayer=physicalPlayer;
                    duelStartRival=input.rival;
                }
                break;
            }
        }
        if(pilotPressed && duelPilotPhase!=DuelPilotPhase::Idle) {
            clearDuelPilot("PREPARACAO CANCELADA PELO JOGADOR");
        } else if(pilotPressed && duelPilotPhase==DuelPilotPhase::Idle &&
            !NativeVehicleFactory::duelStopHeld() && !result.running && !streetSprint.running()) {
            if(!input.challengeSafe || !geometry || input.player.speed>8.0f ||
               input.rival.speed>8.0f) {
                duelPilotCue="ALINHE OS CARROS LADO A LADO DEVAGAR";
                duelPilotCueUntil=GetTickCount64()+4000;
                Log::instance().info("DuelPilot G refused: side_by_side_geometry_or_low_speed_unverified; nativeWrites=0");
            } else if(NativeVehicleFactory::tryAcquireDuelStop()) {
                duelPilotPhase=DuelPilotPhase::Slowing;
                duelPilotElapsed=0;duelPilotCountdown=0;
                duelPilotCue="CARROS RECEBERAM PARADA NATIVA";
                duelPilotCueUntil=GetTickCount64()+4000;
                Log::instance().warn("DuelPilot G accepted: two-car native ForceStop trial; no native input block or race scoring.");
            } else {
                duelPilotCue="PARADA NATIVA NAO AUTORIZADA";
                duelPilotCueUntil=GetTickCount64()+4000;
                Log::instance().warn("DuelPilot G refused: both-car native stop failed; no fake countdown.");
            }
        }
        if(duelPilotPhase!=DuelPilotPhase::Idle) {
            duelPilotElapsed+=std::clamp(dt,0.0f,0.5f);
            if(!NativeVehicleFactory::duelStopHeld() || !input.challengeSafe ||
               duelPilotElapsed>9.0f) {
                clearDuelPilot("PARADA INTERROMPIDA - LIBERANDO CARROS");
            } else if(duelPilotPhase==DuelPilotPhase::Slowing) {
                if(input.player.speed<1.0f && input.rival.speed<1.0f) {
                    duelPilotPhase=DuelPilotPhase::Countdown;duelPilotCountdown=0;
                    Log::instance().info("DuelPilot both physical vehicles stopped: trial countdown armed.");
                } else if(duelPilotElapsed>4.0f) clearDuelPilot("CARROS NAO PARARAM - LIBERANDO");
            } else if(duelPilotPhase==DuelPilotPhase::Countdown) {
                if(input.player.speed>=1.5f || input.rival.speed>=1.5f || !geometry) {
                    clearDuelPilot("ALINHAMENTO PERDIDO - LIBERANDO");
                } else {
                    duelPilotCountdown+=std::clamp(dt,0.0f,0.5f);
                    if(duelPilotCountdown>=3.0f) duelPilotPhase=DuelPilotPhase::Releasing;
                }
            }
            if(duelPilotPhase==DuelPilotPhase::Releasing) {
                if(NativeVehicleFactory::releaseDuelStop()) {
                    duelPilotPhase=DuelPilotPhase::Idle;
                    // These readings come from the last aligned, physically
                    // stopped frame before releasing the native brake lease.
                    const bool started=geometry &&
                        duelStartPlayer.speed<1.0f && duelStartRival.speed<1.0f &&
                        streetSprint.begin(duelStartPlayer,duelStartRival);
                    duelPilotCue=started ? "GO! SPRINT RETO COM META REAL" :
                        "LARGADA LIBERADA - META NAO CONFIRMADA";
                    duelPilotCueUntil=GetTickCount64()+4000;
                    Log::instance().info(started ?
                        "StreetSprint GO: two ForceStop flags restored; REAL shared 120m finish armed; vanilla race event=0." :
                        "StreetSprint GO: both ForceStop flags restored but shared finish rejected; no false scoring.");
                }
            }
        }
    }
    if(streetSprint.running()) {
        const auto finished=streetSprint.tick({input.player,input.rival,dt,
            valid&&input.challengeSafe,false});
        recordSprint(finished);
    }
    // Native Racer may stop for traffic or because its goal cannot recover.
    // Do not declare a cause or mutate native controls from this observation.
    const auto stationary=stationaryMonitor.update(
        {r.center.x/unit,r.center.y/unit,r.center.z/unit},rival.speed,dt,
        valid&&rival.active&&!rival.loading&&!rival.destroyed);
    if(stationary.changed) {
        Log::instance().warn(std::string("Rico stationary observation: potentiallyBlocked=")+
            (stationary.stationaryLong?"1":"0")+
            " secondsNearAnchor="+std::to_string(stationary.secondsNearAnchor)+
            " nativeSteeringWrites=0 nativeReset=0");
    }
    const auto reaction=reactionMonitor.update({input.player,input.rival,dt,valid,
        result.running,config.enabled && result.available,result.remainingCooldown});
    if(reaction.changed) {
        Log::instance().info(std::string("RivalAwareness observedState=")+domain::rivalReactionLabel(reaction.state)+
            " nativeAIWrites=0");
    }
    const auto now=GetTickCount64();
    if(result.leaderChanged) {
        lastOvertakeAt=now;lastOvertakeLeader=result.leader;
        Log::instance().info(std::string("LiveEncounter verified close overtake: newLeader=")+
            domain::outrunLeaderName(result.leader)+" nativeAIWrites=0");
    }
    if(result.running&&now-lastRaceLog>=1000) {
        lastRaceLog=now;std::ostringstream log;
        log<<"LiveEncounter race leadMeters="<<result.signedLeadMeters<<" routeMatched="<<result.routeMatched
            <<" dtSeconds="<<dt<<" reason="<<result.reason<<" leader="<<domain::outrunLeaderName(result.leader)<<" hold="<<result.holdProgress<<" elapsed="<<result.elapsedSeconds;
        Log::instance().info(log.str());
    }
    auto view=panel();
    view.rivalStationary=stationary.stationaryLong;
    // The primary vehicle's physics orientation remains meaningful when
    // the player stops. Do not rely on the stale previous velocity vector.
    // These 2D indicators never claim membership in the game's minimap.
    domain::EncounterPoint beaconForward{};
    bool headingVerified=false;
    const auto playerFleet=VehicleSpatialProbe::sample();
    if(playerFleet.registryComplete&&world.vehicles.playerIVehicle) {
        unsigned matches=0;
        for(const auto& box:playerFleet.boxes) {
            if(box.identity!=world.vehicles.playerIVehicle||!box.valid) continue;
            if(std::hypot(box.center.x-p.x,box.center.z-p.z)>5*unit||
               std::abs(box.center.y-p.y)>5*unit) continue;
            beaconForward={box.forward.x,box.forward.y,box.forward.z};
            headingVerified=true;++matches;
        }
        if(matches!=1) headingVerified=false;
    }
    const domain::EncounterPoint playerMetres{p.x/unit,p.y/unit,p.z/unit};
    if(headingVerified) {
        const auto rivalBeacon=domain::directionBeacon(playerMetres,beaconForward,
            input.rival.position);
        if(rivalBeacon.valid) {
            view.rivalBeaconValid=true;
            view.rivalFront=rivalBeacon.front;view.rivalRight=rivalBeacon.right;
            view.rivalMeters=rivalBeacon.distanceMeters;
        }
        if(streetSprint.running()) {
            const auto finish=domain::directionBeacon(playerMetres,beaconForward,
                streetSprint.view().finishPoint);
            if(finish.valid) {
                view.routeCueValid=true;
                view.routeFront=finish.front;view.routeRight=finish.right;
                view.routeMeters=finish.distanceMeters;
            }
        }
        if(result.running&&result.observedTrailCueValid) {
            const auto cue=domain::directionBeacon(playerMetres,beaconForward,
                result.observedTrailCue);
            if(cue.valid) {
                view.routeCueValid=true;
                view.routeFront=cue.front;view.routeRight=cue.right;
                view.routeMeters=cue.distanceMeters;
            }
        }
    }
    line(view.lines[5],"VITORIAS %llu / DERROTAS %llu",static_cast<unsigned long long>(history.wins),
        static_cast<unsigned long long>(history.losses));
    if(result.running) {
        if(lastOvertakeAt && now-lastOvertakeAt<2500)
            line(view.lines[1],"%s",lastOvertakeLeader==domain::OutrunLeader::Player?
                "VOCE ULTRAPASSOU RICO":"RICO ULTRAPASSOU VOCE");
        else line(view.lines[1],"OUTRUN LIVRE - %s",result.leader==domain::OutrunLeader::Player?"VOCE LIDERA":"RICO LIDERA");
        line(view.lines[2],"VANTAGEM %.0f / %.0f METROS",std::abs(result.signedLeadMeters),config.tuning.winLeadMeters);
        line(view.lines[3],"%s",
            !result.routeMatched?"TRILHA DIFERENTE - REENCONTRE A ROTA":
            result.observedTrailCueValid?"SETA AMARELA = TRILHA REAL DO RICO":
            "MESMA TRAJETORIA - MANTENHA A VANTAGEM");
        line(view.lines[4],"TEMPO %.0f S - RIVAL CONTINUA APOS CORRIDA",result.elapsedSeconds);
        view.playerLeading=result.leader==domain::OutrunLeader::Player;view.holdProgress=result.holdProgress;
    } else {
        if(modeToggleAt&&now-modeToggleAt<4000&&!config.nativeStreetDuelPilotEnabled)
            line(view.lines[1],"MODO OUTRUN ATIVADO - SEGUIR RICO");
        else if(lastResultAt&&now-lastResultAt<8000) line(view.lines[1],"%s",resultText(lastResult));
        else if(stationary.stationaryLong)
            line(view.lines[1],"RICO QUASE PARADO HA %.0f S",stationary.secondsNearAnchor);
        else line(view.lines[1],"%s / %.0f KM/H",domain::rivalReactionLabel(reaction.state),rival.speed*3.6f);
        view.challengeReady=config.enabled&&result.available;
        if(view.challengeReady) line(view.lines[2],"[%c] OUTRUN: SEGUIR RICO (SEM LARGADA)",static_cast<char>(ChallengeInputProbe::fallbackVirtualKey()));
        else if(!config.enabled) line(view.lines[2],"DESAFIOS DESATIVADOS");
        else if(result.remainingCooldown>0) line(view.lines[2],"NOVO DESAFIO EM %.0f S",result.remainingCooldown);
        else if(!input.challengeSafe) line(view.lines[2],"DESAFIO BLOQUEADO: POLICIA OU ESTADO INCERTO");
        else line(view.lines[2],"%s",rejectionText(result.reason));
        if(lastRejectionAt&&now-lastRejectionAt<3000) line(view.lines[2],"%s",rejectionText(lastRejection));
        // Straight-line location from current, valid world positions; no guessed route.
        const auto locator=domain::locateRival(
            {input.player.position.x,input.player.position.y,input.player.position.z},
            {input.rival.position.x,input.rival.position.y,input.rival.position.z},
            {input.player.forward.x,input.player.forward.y,input.player.forward.z},
            // Do not present stale lastPlayerForward when current velocity is absent.
            velocity.finite && std::isfinite(horizontal) && horizontal>0.01f &&
            std::isfinite(input.player.speed) && input.player.speed>=3.0f);
        if(locator.valid) {
            if(locator.bearing==domain::RivalBearing::Unknown)
                line(view.lines[3],"RICO %.0f M / MOVA-SE PARA VER DIRECAO",locator.distanceMeters);
            else line(view.lines[3],"RICO %.0f M - %s",locator.distanceMeters,domain::rivalBearingName(locator.bearing));
        } else line(view.lines[3],"LOCALIZACAO DO RIVAL INDISPONIVEL");
        line(view.lines[4],"%s",nativeRivalRetirementHint());
    }
    // Preparatory read-only staging: both vehicles must be physically
    // side-by-side with compatible headings and almost fully stopped.
    // v53 does NOT seize player controls or native AI brakes. Present a
    // truthful lineup message; automatic countdown is not authorized until
    // native custody/rollback operations have been verified.
    if(!result.running&&headingVerified&&input.rival.valid) {
        auto startPlayer=input.player;
        startPlayer.forward=beaconForward;
        startPlayer.valid=true;
        const auto formation=domain::checkRaceFormation(startPlayer,input.rival,
            valid&&input.challengeSafe);
        if(formation.geometryAligned&&formation.bothStopped) {
            line(view.lines[3],"CARROS ALINHADOS - RICO AO LADO");
            line(view.lines[4],"LARGADA AUTOMATICA AINDA INDISPONIVEL");
        }
    }
    if(config.nativeStreetDuelPilotEnabled) {
        view.challengeReady=false;
        if(streetSprint.running()) {
            const auto& sprint=streetSprint.view();
            const bool playerAhead=sprint.playerMeters>=sprint.rivalMeters;
            view.playerLeading=playerAhead;
            view.holdProgress=std::clamp(
                std::max(sprint.playerMeters,sprint.rivalMeters)/
                    std::max(sprint.distanceMeters,1.0f),0.0f,1.0f);
            line(view.lines[1],"SPRINT CURTO - %s",playerAhead?"VOCE LIDERA":"RICO LIDERA");
            line(view.lines[2],"PROGRESSO %.0f M / %.0f M",sprint.playerMeters,sprint.distanceMeters);
            line(view.lines[3],"RICO %.0f M - META AMARELA NA DIRECAO",sprint.rivalMeters);
            line(view.lines[4],"TEMPO %.1f S - G CANCELA",sprint.elapsed);
        } else if(lastSprintResultAt&&GetTickCount64()-lastSprintResultAt<8000) {
            const char* outcome=lastSprintOutcome==domain::OutrunOutcome::PlayerWon?
                "SPRINT: VOCE VENCEU":lastSprintOutcome==domain::OutrunOutcome::RivalWon?
                "SPRINT: RICO VENCEU":lastSprintOutcome==domain::OutrunOutcome::Draw?
                "SPRINT: EMPATE":"SPRINT: INTERROMPIDO";
            line(view.lines[1],"%s",outcome);
            line(view.lines[2],"RESULTADO REGISTRADO NO PERFIL");
        } else if(duelPilotPhase==DuelPilotPhase::Slowing) {
            line(view.lines[1],"DUEL PILOTO - PARANDO OS CARROS");
            line(view.lines[2],"AGUARDANDO PARADA FISICA DOS DOIS");
            line(view.lines[3],"G NOVAMENTE CANCELA");
        } else if(duelPilotPhase==DuelPilotPhase::Countdown) {
            const int remaining=std::clamp(3-static_cast<int>(duelPilotCountdown),1,3);
            line(view.lines[1],"DUEL PILOTO - LARGADA EM %d",remaining);
            line(view.lines[2],"SEM CUTSCENE - CARROS PARADOS");
            line(view.lines[3],"G CANCELA E LIBERA");
        } else if(duelPilotPhase==DuelPilotPhase::Releasing) {
            line(view.lines[1],"LIBERANDO OS DOIS CARROS");
            line(view.lines[2],"AGUARDANDO CONFIRMACAO NATIVA");
        } else if(GetTickCount64()<duelPilotCueUntil) {
            line(view.lines[1],"%s",duelPilotCue);
            line(view.lines[2],"DUELO COM META FISICA DE %.0f M",config.streetSprintLengthMeters);
        } else if(!result.running) {
            line(view.lines[2],"G: INICIAR SPRINT RETO (LADO A LADO)");
        }
    }
    if(!config.nativeStreetDuelPilotEnabled&&!result.running)
        line(view.lines[4],"CTRL+SHIFT+G ALTERNA SPRINT / OUTRUN");
    publishNativeRivalHud(view);
}
}
