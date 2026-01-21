#include <stdint.h>
#include "visual.h"
#ifndef SIM_CONFIG_H
    #define SIM_CONFIG_H
    const float smoothingRadius;
    const float targetDensity;
    const float Poly6ScalingFactor;
    const float SpikyPow3ScalingFactor;
    const float SpikyPow2ScalingFactor;
    const float SpikyPow3DerivativeScalingFactor;
    const float SpikyPow2DerivativeScalingFactor;
    
    // les buffers fait autrement 
    Vector2* Positions;
    Vector2* PredictedPositions;
    Vector2* Velocities;
    Vector2* Densities; // Density, Near Density

    // Spatial hashing
    uint32_t* SpatialKeys;
    uint32_t* SpatialOffsets;
    uint32_t* SortedIndices;

    // Settings
    const int32_t numParticles;
    const float gravity;
    const float deltaTime;
    const float collisionDamping;
    
    const float pressureMultiplier;
    const float nearPressureMultiplier;
    const float viscosityStrength;
    const Vector2 boundsSize;
    const Vector2 interactionInputPoint;
    const float interactionInputStrength;
    const float interactionInputRadius;

    const Vector2 obstacleSize;
    const Vector2 obstacleCentre;


    // ------------------ hashing Part -------------------
    static const uint32_t hashK1 = 15823;
    static const uint32_t hashK2 = 9737333;
    static const Vector2 offsets2D[9] =
    {
        {-1, 1},
        {0, 1},
        {1, 1},
        {-1, 0},
        {0, 0},
        {1, 0},
        {-1, -1},
        {0, -1},
        {1, -1}
    };

    Vector2* SortTarget_Positions;
    Vector2* SortTarget_PredictedPositions;
    Vector2* SortTarget_Velocities;

    // -----------------   functions ------------------------
    Vector2 GetCell2D(Vector2 position, float radius);
    int sign(float x);
    uint32_t HashCell2D(Vector2 cell);
    uint32_t KeyFromHash(uint32_t hash, uint32_t tableSize);
    float SmoothingKernelPoly6(float dst, float radius);
    float SpikyKernelPow3(float dst, float radius);
    float SpikyKernelPow2(float dst, float radius);
    float DerivativeSpikyPow3(float dst, float radius);
    float DerivativeSpikyPow2(float dst, float radius);
    float DensityKernel(float dst, float radius);
    float NearDensityKernel(float dst, float radius);
    float DensityDerivative(float dst, float radius);
    float NearDensityDerivative(float dst, float radius);
    float ViscosityKernel(float dst, float radius);
    Vector2 CalculateDensity(Vector2 pos);
    float PressureFromDensity(float density);
    float NearPressureFromDensity(float nearDensity);
    Vector2 ExternalForces(Vector2 pos, Vector2 velocity);
    void HandleCollisions(uint32_t particleIndex);
    void calcExternalForces(Vector2 *positions,Vector2 *velocities, Vector2 *predictedPositions,int numParticles,float deltaTime);
    void UpdateSpatialHash(Vector2 *predictedPositions, uint32_t *spatialKeys, int numParticles, float smoothingRadius);
    void Reorder(Vector2 *positions,Vector2 *predictedPositions,Vector2 *velocities,uint32_t *sortedIndices,Vector2 *sortTarget_Positions,Vector2 *sortTarget_PredictedPositions,Vector2 *sortTarget_Velocities,int numParticles);
    void ReorderCopyback(Vector2 *positions,Vector2 *predictedPositions,Vector2 *velocities,Vector2 *sortTarget_Positions,Vector2 *sortTarget_PredictedPositions,Vector2 *sortTarget_Velocities,int numParticles);
    void  CalculateDensities(Vector2 *predictedPositions, float *densities, int numParticles);
    void CalculatePressureForce(int id);
    void CalculateViscosity(int id);
    void UpdatePositions(int id);

#endif
