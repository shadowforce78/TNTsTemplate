#pragma once
#include "Modules/Module.hpp"
#include "Components/Includes.hpp"
#include <vector> // Required for std::vector
#include <mutex>
#include <string>

struct CarBoostData {
    FVector screenPosition;
    float boostAmount;
};

struct CachedBallData {
    FVector Location;
    FVector Velocity;
    float Radius;
};

struct CachedCarData {
    FVector Location;
    float Boost;
    std::string Name;
    void* Ptr; // ID for ImGui
};

class ExampleModule : public Module
{
public:
    ExampleModule();
    ~ExampleModule() override;

    void OnCreate();
    void OnDestroy();
    void OnRender(); // Add the OnRender function declaration

    static void Hook();

    static void OnGameEventStart(PreEvent& event);
    static void OnGameEventDestroyed(PreEvent& event);
    static void PlayerTickCalled(const PostEvent& event);

    static void Initialize();

    static bool IsInGame;
    static AGameEvent_Soccar_TA* CurrentGameEvent;

    struct BallPath {
        std::vector<FVector> points;
        bool willScore;
    };
    static std::vector<BallPath> ballPaths;
    
    // Thread Safety
    static std::mutex renderMutex;
    static std::vector<CachedBallData> cachedBalls;
    static std::vector<CachedCarData> cachedCars;

    // Static vectors to store calculated screen positions
    static std::vector<CarBoostData> carBoostData;
    static std::vector<FVector> ballScreenPositions;
    static APRI_TA* localPlayerPRI;
};

extern class ExampleModule Example;