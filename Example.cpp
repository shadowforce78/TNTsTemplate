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
std::vector<ExampleModule::BallPath> ExampleModule::ballPaths;
std::mutex ExampleModule::renderMutex;
std::vector<CachedBallData> ExampleModule::cachedBalls;
std::vector<CachedCarData> ExampleModule::cachedCars;

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
        ballPaths.clear();
        std::lock_guard<std::mutex> lock(renderMutex);
        cachedBalls.clear();
        cachedCars.clear();
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

    // Use local temp vectors and swap at the end to avoid locking the entire function
    // carBoostData.clear();
    // ballScreenPositions.clear();
    // ballPaths.clear();

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
    // Temp vectors for thread safety
    std::vector<CachedCarData> tempCars;
    std::vector<CachedBallData> tempBalls;
    std::vector<CarBoostData> tempCarBoostData;
    std::vector<BallPath> tempBallPaths;
    std::vector<FVector> tempBallScreenPositions;

    // get all cars with boost data
    for (ACar_TA* car : cars) {
        if (!car) continue;

        FVector carLocation = car->Location;
        FVector boostCircleLocation = carLocation;
        boostCircleLocation.Z += 100.0f; 

        // Screen pos for overlay
        FVector screenPos = Drawing::CalculateScreenCoordinate(boostCircleLocation, localPlayerController);

        ACarComponent_Boost_TA* boostComponent = car->BoostComponent;
        float boostAmount = 0.0f;
        if (boostComponent) {
            boostAmount = boostComponent->CurrentBoostAmount; // * 100 for display?
        }

        CarBoostData data;
        data.screenPosition = screenPos;
        data.boostAmount = boostAmount;
        tempCarBoostData.push_back(data);

        // Cache for GUI
        CachedCarData cacheData;
        cacheData.Location = carLocation;
        cacheData.Boost = boostAmount;
        cacheData.Ptr = (void*)car;
        
        if (car->PRI && car->PRI->PlayerName.length() > 0) {
             cacheData.Name = car->PRI->PlayerName.ToString();
        } else {
             cacheData.Name = "Car";
        }
        tempCars.push_back(cacheData);
    }

    // get all balls and save in list
    if (balls.size() > 0) {
        Console.Write("Found " + std::to_string(balls.size()) + " balls"); 
    }
    for (ABall_TA* ball : balls) {
        if (!ball) continue;
        if (ball->Location.X == 0.0f && ball->Location.Y == 0.0f && ball->Location.Z == 0.0f) continue;
        
        // Cache for GUI
        CachedBallData cacheData;
        cacheData.Location = ball->Location;
        cacheData.Velocity = ball->Velocity;
        cacheData.Radius = ball->Radius;
        tempBalls.push_back(cacheData);

        FVector ballLocation = ball->Location;
        FVector screenPos = Drawing::CalculateScreenCoordinate(ballLocation, localPlayerController);
        tempBallScreenPositions.push_back(screenPos);

        // Manual Prediction
        BallPath path;
        path.willScore = false;
        
        // Initial State
        // Use SDK Getters for better reliability
        // Use direct access for debugging
        FVector simLoc = ball->Location;
        FVector simVel = ball->Velocity; 
        
        // Debug
        // Console.Write("Ball Vel: " + std::to_string(simVel.Z));

        // Physics Constants
        
        // Physics Constants
        const float GRAVITY_Z = -650.0f;
        const float DT = 0.1f;
        const float BALL_RADIUS = 93.0f; // Approx ball radius
        const float RESTITUTION = 0.6f; // Bounce dampening

        // Add start point
        path.points.push_back(Drawing::CalculateScreenCoordinate(simLoc, localPlayerController));

        // Simulate 4 seconds
        for (int i = 0; i < 40; i++) {
            // Apply Gravity
            simVel.Z += GRAVITY_Z * DT;

            // Apply Velocity
            simLoc.X += simVel.X * DT;
            simLoc.Y += simVel.Y * DT;
            simLoc.Z += simVel.Z * DT;

            // Floor Bounce
            if (simLoc.Z < BALL_RADIUS) {
                simLoc.Z = BALL_RADIUS;
                simVel.Z *= -RESTITUTION;
            }

            // Wall Bounces (Simplified: just clamp for now, or ignore)
            // Ideally we'd raycast or check bounds, but let's stick to gravity/floor first.

            // Convert to screen
            FVector predictedScreenPos = Drawing::CalculateScreenCoordinate(simLoc, localPlayerController);
            path.points.push_back(predictedScreenPos);

            // Goal detection
            // Goal Y is approx +/- 5120, Width X +/- 900, Height Z < 650
            if (abs(simLoc.Y) > 5120.0f && abs(simLoc.X) < 900.0f && simLoc.Z < 650.0f) {
                path.willScore = true;
            }
        }
        
        tempBallPaths.push_back(path);
    }

    // UPDATE SHARED DATA SAFELY
    {
        std::lock_guard<std::mutex> lock(renderMutex);
        cachedBalls = tempBalls;
        cachedCars = tempCars;
        ballPaths = tempBallPaths;
        carBoostData = tempCarBoostData;
        ballScreenPositions = tempBallScreenPositions;
    }
}

void ExampleModule::OnRender() {
    // Watermark "SushiSDK" - Always visible
    {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const char* text = "SushiSDK";
        ImVec2 pos(30, 30);
        ImU32 glowColor = IM_COL32(0, 255, 255, 100); // Cyan glow
        ImU32 textColor = IM_COL32(255, 255, 255, 255);
        
        // Glow/Outline effect
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                if (x == 0 && y == 0) continue;
                drawList->AddText(ImGui::GetFont(), 24.0f, ImVec2(pos.x + x, pos.y + y), glowColor, text);
            }
        }
        drawList->AddText(ImGui::GetFont(), 24.0f, pos, textColor, text);
    }

    if (!IsInGame) {
        return;
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Local copy for rendering to avoid long locks or race conditions
    std::vector<BallPath> localBallPaths;
    std::vector<CarBoostData> localCarBoostData;
    std::vector<FVector> localBallScreenPositions;
    {
        std::lock_guard<std::mutex> lock(renderMutex);
        localBallPaths = ballPaths;
        localCarBoostData = carBoostData;
        localBallScreenPositions = ballScreenPositions;
    }

    // Draw Prediction Lines
    for (const BallPath& path : localBallPaths) {
        ImU32 color = path.willScore ? IM_COL32(255, 0, 0, 255) : IM_COL32(255, 255, 0, 255);
        
        if (path.points.size() < 2) continue;

        for (size_t i = 0; i < path.points.size() - 1; i++) {
            FVector p1 = path.points[i];
            FVector p2 = path.points[i+1];

            // Valid check (Z==0 means on screen for our function)
            if (p1.Z == 0.0f && p2.Z == 0.0f) {
                 if (std::isfinite(p1.X) && std::isfinite(p1.Y) && std::isfinite(p2.X) && std::isfinite(p2.Y)) {
                     drawList->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, 3.0f);
                 }
            }
        }
    }

    /* Boost Render Disabled
    // ...
    */

    // ball circles
    for (const FVector& screenPos : localBallScreenPositions) {
        if (screenPos.Z == 0) {
            drawList->AddCircleFilled(ImVec2(screenPos.X, screenPos.Y), 8.f, IM_COL32(0, 255, 0, 255));
        }
    }
}

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

