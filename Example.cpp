#include "Example.hpp"
#include "Offsets.hpp" // using pointers is the most straightforward, safest and easist
#include "Modules/Mods/Drawing.hpp"
#include "ImGui/imgui.h"
#include <algorithm>
#include <cmath>

#ifndef IM_PI
#define IM_PI 3.14159265358979323846f
#endif

bool ExampleModule::IsInGame = false;
AGameEvent_Soccar_TA* ExampleModule::CurrentGameEvent = nullptr;
std::vector<CarBoostData> ExampleModule::carBoostData;
std::vector<FVector> ExampleModule::ballScreenPositions;
APRI_TA* ExampleModule::localPlayerPRI = nullptr;

void ExampleModule::Hook() {
    Events.HookEventPre("Function TAGame.GameEvent_Soccar_TA.PostBeginPlay", OnGameEventStart);
    Events.HookEventPre("Function TAGame.GameEvent_Soccar_TA.Destroyed", OnGameEventDestroyed);
    Events.HookEventPre("Function TAGame.GameEvent_Soccar_TA.Active.BeginState", OnGameEventStart);
    Events.HookEventPre("Function TAGame.GameEvent_Soccar_TA.Countdown.BeginState", OnGameEventStart);
    Events.HookEventPost("Function TAGame.PlayerController_TA.PlayerTick", PlayerTickCalled);
}

void ExampleModule::OnGameEventDestroyed(PreEvent& event)
{
    try
    {
        CurrentGameEvent = nullptr;
        IsInGame = false;
        carBoostData.clear();
        ballScreenPositions.clear();
    }
    catch (...) { Console.Error("GameEventHook: Exception in OnGameEventDestroyed"); }
}

void ExampleModule::OnGameEventStart(PreEvent& event)
{
    try
    {
        Console.Write("GameEventHook: Game event started: " + std::string(event.Function()->GetName()));
        if (event.Caller() && event.Caller()->IsA(AGameEvent_Soccar_TA::StaticClass()))
        {
            CurrentGameEvent = static_cast<AGameEvent_Soccar_TA*>(event.Caller());
            Console.Write("GameEventHook: Stored GameEvent instance");
        }
        IsInGame = true;
    }
    catch (...) { Console.Error("GameEventHook: Exception in OnGameEventStart"); }
}

void ExampleModule::PlayerTickCalled(const PostEvent& event) {
    if (!IsInGame || !CurrentGameEvent || !event.Caller() || !event.Caller()->IsA(APlayerController_TA::StaticClass())) {
        return;
    }

    carBoostData.clear();
    ballScreenPositions.clear();

    TArray<APlayerController_TA*> localPlayers = CurrentGameEvent->LocalPlayers;
    if (localPlayers.size() == 0 || !localPlayers[0]) {
        return;
    }
    APlayerController_TA* localPlayerController = localPlayers[0];
    localPlayerPRI = localPlayerController->PRI;

    TArray<ACar_TA*> cars = CurrentGameEvent->Cars;
    TArray<ABall_TA*> balls = CurrentGameEvent->GameBalls;

    for (APlayerController_TA* localPlayer : localPlayers) {
        // example for getting Car/PRI
        ACar_TA* car = localPlayer->Car;
        APRI_TA* PRI = localPlayer->PRI;

        //get input
        //FVehicleInputs currentInputs = localPlayer->VehicleInput; //might not work
    }

    // get all cars with boost data
    for (ACar_TA* car : cars) {
        if (!car) continue;

        FVector carLocation = car->Location;

        FVector boostCircleLocation = carLocation;
        boostCircleLocation.Z += 100.0f; 

        FVector screenPos = Drawing::CalculateScreenCoordinate(boostCircleLocation, localPlayerController);


        ACarComponent_Boost_TA* boostComponent = nullptr;
        
        // Debugging: List everything attached
        /*
        Console.Write("Car Attached Count: " + std::to_string(car->Attached.Count));
        for (int i = 0; i < car->Attached.Count; i++) {
            AActor* actor = car->Attached[i];
            if (actor) {
                Console.Write("  Attached[" + std::to_string(i) + "]: " + actor->GetFullName());
                if (actor->IsA(ACarComponent_Boost_TA::StaticClass())) {
                     Console.Write("    -> MATCHES Boost Class!");
                     boostComponent = static_cast<ACarComponent_Boost_TA*>(actor);
                }
            }
        }
        */

        // Explicit search with logging enabled for the user
    if (!Canvas) return;

    // Project is clean.
    // Logic removed as requested.

    if (!IsInGame) {
        return;
    }
    
    // Clean render loop.

ExampleModule::ExampleModule() : Module("GameEventHook", "Hooks into game events", States::STATES_All) {
    OnCreate();
}
ExampleModule::~ExampleModule() { OnDestroy(); }

void ExampleModule::OnCreate() {
    // do non
}

void ExampleModule::OnDestroy() {
    // do non
}

void ExampleModule::Initialize() {
    Hook();
    Console.Write("ExampleModule Initalized.");
}


ExampleModule Example;

