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

    // Clean up lists
    carBoostData.clear();
    ballScreenPositions.clear();
}

void ExampleModule::OnRender() {
    // Show on ALL screens (Menu, Loading, Game) - check removed.

    // --- SushiSDK Badge Implementation ---
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    float time = (float)ImGui::GetTime();

    // Configuration
    const char* text = "SushiSDK";
    float paddingX = 15.0f;
    float paddingY = 8.0f;
    float cornerRadius = 8.0f;
    float topMargin = 20.0f;
    float leftMargin = 20.0f; // Moved to Left

    // Pulse effect (0.0 to 1.0) for breathing
    float pulse = (sinf(time * 3.0f) * 0.5f) + 0.5f; 

    // Calculate size & position
    // Note: We use default font size. For larger text we'd need to scale or push font.
    // Keeping it simple and clean with default size usually looks best unless high DPI issues occur.
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImVec2 boxSize = ImVec2(textSize.x + paddingX * 2, textSize.y + paddingY * 2);
    
    // Position: Top-Left
    ImVec2 pos = ImVec2(leftMargin, topMargin);

    // Colors
    // Sexy Dark Background
    ImU32 bgColor = IM_COL32(20, 20, 20, 180); 
    // Sushi Salmon Pink Border (255, 114, 118)
    // Pulsating Alpha for border
    ImU32 borderColor = IM_COL32(255, 114, 118, 150 + (int)(pulse * 105)); 
    
    // Draw Glow (Outer Stroke Loop)
    // Create a "luminous" effect by drawing expanding shells with decreasing alpha
    int glowPasses = 6;
    for (int i = 0; i < glowPasses; i++) {
        float expand = (i + 1) * 1.5f;
        float baseAlpha = 100.0f; // Max glow alpha
        float glowAlpha = (baseAlpha * (1.0f - ((float)i / glowPasses))) * pulse; 
        
        drawList->AddRect(
            ImVec2(pos.x - expand, pos.y - expand),
            ImVec2(pos.x + boxSize.x + expand, pos.y + boxSize.y + expand),
            IM_COL32(255, 114, 118, (int)glowAlpha),
            cornerRadius + expand, 
            0, 
            1.0f // Thickness
        );
    }

    // Draw Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + boxSize.x, pos.y + boxSize.y), bgColor, cornerRadius);
    
    // Draw Border
    drawList->AddRect(pos, ImVec2(pos.x + boxSize.x, pos.y + boxSize.y), borderColor, cornerRadius, 0, 1.5f);

    // Draw Text with slight shadow for pop
    drawList->AddText(ImVec2(pos.x + paddingX + 1, pos.y + paddingY + 1), IM_COL32(0, 0, 0, 150), text);
    drawList->AddText(ImVec2(pos.x + paddingX, pos.y + paddingY), IM_COL32(255, 230, 230, 255), text);
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

