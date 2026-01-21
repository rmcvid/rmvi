#include "fluid_seb.h"
// Constants used for hashing


// Convert floating point position into an integer cell coordinate
Vector2 GetCell2D(Vector2 position, float radius)
{
	return (Vector2){ floor(position.x / radius), floor(position.y / radius)};
}
int sign(float x){
    int res = x == 0 ?  1 :  x/abs(x) ;
    return res;
     
}
// Hash cell coordinate to a single unsigned integer
uint32_t HashCell2D(Vector2 cell)
{
	uint32_t a = (uint32_t)cell.x * hashK1;
	uint32_t b = (uint32_t)cell.y * hashK2;
	return (a + b);
}

uint32_t KeyFromHash(uint32_t hash, uint32_t tableSize)
{
	return hash % tableSize;
}


// --------------------- Fluid Math Part -----------------------
float SmoothingKernelPoly6(float dst, float radius)
{
	if (dst < radius)
	{
		float v = radius * radius - dst * dst;
		return v * v * v * Poly6ScalingFactor;
	}
	return 0;
}

float SpikyKernelPow3(float dst, float radius)
{
	if (dst < radius)
	{
		float v = radius - dst;
		return v * v * v * SpikyPow3ScalingFactor;
	}
	return 0;
}

float SpikyKernelPow2(float dst, float radius)
{
	if (dst < radius)
	{
		float v = radius - dst;
		return v * v * SpikyPow2ScalingFactor;
	}
	return 0;
}

float DerivativeSpikyPow3(float dst, float radius)
{
	if (dst <= radius)
	{
		float v = radius - dst;
		return -v * v * SpikyPow3DerivativeScalingFactor;
	}
	return 0;
}

float DerivativeSpikyPow2(float dst, float radius)
{
	if (dst <= radius)
	{
		float v = radius - dst;
		return -v * SpikyPow2DerivativeScalingFactor;
	}
	return 0;
}

float DensityKernel(float dst, float radius)
{
	return SpikyKernelPow2(dst, radius);
}

float NearDensityKernel(float dst, float radius)
{
	return SpikyKernelPow3(dst, radius);
}

float DensityDerivative(float dst, float radius)
{
	return DerivativeSpikyPow2(dst, radius);
}

float NearDensityDerivative(float dst, float radius)
{
	return DerivativeSpikyPow3(dst, radius);
}

float ViscosityKernel(float dst, float radius)
{
	return SmoothingKernelPoly6(dst, smoothingRadius);
}

Vector2 CalculateDensity(Vector2 pos)
{
	Vector2 originCell = GetCell2D(pos, smoothingRadius);
	float sqrRadius = smoothingRadius * smoothingRadius;
	float density = 0;
	float nearDensity = 0;

	// Neighbour search
	for (int i = 0; i < 9; i++)
	{
		uint32_t hash = HashCell2D( Vector2Add( originCell, offsets2D[i]));
		uint32_t key = KeyFromHash(hash, numParticles);
		uint32_t currIndex = SpatialOffsets[key];

		while (currIndex < numParticles)
		{
			uint32_t neighbourIndex = currIndex;
			currIndex ++;
			
			uint32_t neighbourKey = SpatialKeys[neighbourIndex];
			// Exit if no longer looking at correct bin
			if (neighbourKey != key) break;

			Vector2 neighbourPos = PredictedPositions[neighbourIndex];
			Vector2 offsetToNeighbour = Vector2Subtract(neighbourPos, pos);
			float sqrDstToNeighbour = Vector2DotProduct(offsetToNeighbour, offsetToNeighbour);

			// Skip if not within radius
			if (sqrDstToNeighbour > sqrRadius) continue;

			// Calculate density and near density
			float dst = sqrt(sqrDstToNeighbour);
			density += DensityKernel(dst, smoothingRadius);
			nearDensity += NearDensityKernel(dst, smoothingRadius);
		}
	}

	return (Vector2){density, nearDensity};
}

float PressureFromDensity(float density)
{
	return (density - targetDensity) * pressureMultiplier;
}

float NearPressureFromDensity(float nearDensity)
{
	return nearPressureMultiplier * nearDensity;
}

Vector2 ExternalForces(Vector2 pos, Vector2 velocity)
{
	// Gravity
	Vector2 gravityAccel = {0, gravity};
	
	// Input interactions modify gravity
	if (interactionInputStrength != 0) {
		Vector2 inputPointOffset = Vector2Subtract(interactionInputPoint , pos);
		float sqrDst = Vector2DotProduct(inputPointOffset, inputPointOffset);
		if (sqrDst < interactionInputRadius * interactionInputRadius)
		{
			float dst = sqrt(sqrDst);
			float edgeT = (dst / interactionInputRadius);
			float centreT = 1 - edgeT;
			Vector2 dirToCentre = Vector2Scale(inputPointOffset, 1.0f/dst);

			float gravityWeight = 1 - (centreT * saturate(interactionInputStrength / 10));
			Vector2 accel = Vector2Add( Vector2Scale( gravityAccel, gravityWeight),
                                        Vector2Scale(dirToCentre, centreT * interactionInputStrength)
                            );

			accel = Vector2Subtract(accel, Vector2Scale(velocity , centreT));
			return accel;
		}
	}

	return gravityAccel;
}


void HandleCollisions(uint32_t particleIndex)
{
	Vector2 pos = Positions[particleIndex];
	Vector2 vel = Velocities[particleIndex];

	// Keep particle inside bounds
	const Vector2 halfSize = Vector2Scale(boundsSize, 0.5);
	Vector2 edgeDst = Vector2Subtract( halfSize, (Vector2){abs(pos.x),abs(pos.y)});

	if (edgeDst.x <= 0)
	{
		pos.x = halfSize.x * sign(pos.x);
		vel.x *= -1 * collisionDamping;
	}
	if (edgeDst.y <= 0)
	{
		pos.y = halfSize.y * sign(pos.y);
		vel.y *= -1 * collisionDamping;
	}

	// Collide particle against the test obstacle
	const Vector2 obstacleHalfSize = Vector2Scale(obstacleSize, 0.5);
    Vector2 intermediaire = Vector2Subtract(pos,obstacleCentre);
	Vector2 obstacleEdgeDst = 
                Vector2Subtract( 
                    obstacleHalfSize,
                    (Vector2) {abs(intermediaire.x), abs(intermediaire.y )}
                );

	if (obstacleEdgeDst.x >= 0 && obstacleEdgeDst.y >= 0)
	{
		if (obstacleEdgeDst.x < obstacleEdgeDst.y) {
			pos.x = obstacleHalfSize.x * sign(pos.x - obstacleCentre.x) + obstacleCentre.x;
			vel.x *= -1 * collisionDamping;
		}
		else {
			pos.y = obstacleHalfSize.y * sign(pos.y - obstacleCentre.y) + obstacleCentre.y;
			vel.y *= -1 * collisionDamping;
		}
	}

	// Update position and velocity
	Positions[particleIndex] = pos;
	Velocities[particleIndex] = vel;
}

void calcExternalForces(Vector2 *positions,Vector2 *velocities, Vector2 *predictedPositions,int numParticles,float deltaTime){
    const float predictionFactor = 1.0f / 120.0f;

    for (int i = 0; i < numParticles; i++)
    {
        // Ajout des forces extérieures
        Vector2 force = ExternalForces(positions[i], velocities[i]);
        velocities[i] = Vector2Add(velocities[i], Vector2Scale(force, deltaTime));

        // Prédiction (semi-implicit Euler)
        predictedPositions[i] = Vector2Add(
            positions[i],
            Vector2Scale(velocities[i], predictionFactor)
        );
    }
}


void UpdateSpatialHash(Vector2 *predictedPositions, uint32_t *spatialKeys, int numParticles, float smoothingRadius){
    for (uint32_t i = 0; i < (uint32_t)numParticles; i++)
    {
        // Trouve la cellule dans la grille spatiale 2D
        Vector2 cell = GetCell2D(predictedPositions[i], smoothingRadius);

        // Hash integer pour cette cellule
        uint32_t hash = HashCell2D(cell);

        // Dépend du nombre total de particules
        uint32_t key = KeyFromHash(hash, numParticles);

        spatialKeys[i] = key;
    }
}


void Reorder(Vector2 *positions,Vector2 *predictedPositions,Vector2 *velocities,uint32_t *sortedIndices,Vector2 *sortTarget_Positions,Vector2 *sortTarget_PredictedPositions,Vector2 *sortTarget_Velocities,int numParticles)
{
    for (uint32_t i = 0; i < (uint32_t)numParticles; i++)
    {
        uint32_t sortedIndex = sortedIndices[i];
        sortTarget_Positions[i]           = positions[sortedIndex];
        sortTarget_PredictedPositions[i]  = predictedPositions[sortedIndex];
        sortTarget_Velocities[i]          = velocities[sortedIndex];
    }
}


void ReorderCopyback(Vector2 *positions,Vector2 *predictedPositions,Vector2 *velocities,Vector2 *sortTarget_Positions,Vector2 *sortTarget_PredictedPositions,Vector2 *sortTarget_Velocities,int numParticles)
{
    for (int i = 0; i < numParticles; i++)
    {
        positions[i]           = sortTarget_Positions[i];
        predictedPositions[i]  = sortTarget_PredictedPositions[i];
        velocities[i]          = sortTarget_Velocities[i];
    }
}

void  CalculateDensities(Vector2 *predictedPositions, float *densities, int numParticles)
{
    for (int i = 0; i < numParticles; i++)
    {
        Vector2 pos = predictedPositions[i];
        densities[i] = CalculateDensityCPU(pos, predictedPositions, numParticles);
    }
}

void CalculatePressureForce(int id)
{
    if (id >= numParticles) return;

    float density      = Densities[id].x; // ici ?
    float densityNear  = Densities[id].y;
    float pressure     = PressureFromDensity(density);
    float nearPressure = NearPressureFromDensity(densityNear);

    Vector2 pressureForce = Vector2Zero();

    Vector2 pos = PredictedPositions[id];
    Vector2 originCell = GetCell2D(pos, smoothingRadius);
    float sqrRadius = smoothingRadius * smoothingRadius;

    // Recherche de voisins
    for (int i = 0; i < 9; i++)
    {
        uint32_t hash = HashCell2D(Vector2Add(originCell, offsets2D[i]));
        uint32_t key  = KeyFromHash(hash, numParticles);

        uint32_t currIndex = SpatialOffsets[key];

        while (currIndex < numParticles)
        {
            uint32_t neighbourIndex = currIndex;
            currIndex++;

            // Ignorer soi-même
            if (neighbourIndex == id) continue;

            uint32_t neighbourKey = SpatialKeys[neighbourIndex];
            if (neighbourKey != key) break;  // fin du bin

            Vector2 neighbourPos = PredictedPositions[neighbourIndex];
            Vector2 offsetToNeighbour = Vector2Subtract(neighbourPos, pos);

            float sqrDst = Vector2DotProduct(offsetToNeighbour, offsetToNeighbour);
            if (sqrDst > sqrRadius) continue;

            float dst = sqrtf(sqrDst);

            Vector2 dirToNeighbour =
                (dst > 0.0f) ? Vector2Scale(offsetToNeighbour, 1.0f / dst)
                             : (Vector2){0, 1};

            float nDensity      = Densities[neighbourIndex].x;
            float nDensityNear  = Densities[neighbourIndex].y;
            float nPressure     = PressureFromDensity(nDensity);
            float nNearPressure = NearPressureFromDensity(nDensityNear);

            float sharedPressure     = (pressure     + nPressure)     * 0.5f;
            float sharedNearPressure = (nearPressure + nNearPressure) * 0.5f;

            // Pressure force
            float d = DensityDerivative(dst, smoothingRadius);
            pressureForce = Vector2Add(
                pressureForce,
                Vector2Scale(dirToNeighbour, d * sharedPressure / nDensity)
            );

            // Near pressure force
            float dn = NearDensityDerivative(dst, smoothingRadius);
            pressureForce = Vector2Add(
                pressureForce,
                Vector2Scale(dirToNeighbour, dn * sharedNearPressure / nDensityNear)
            );
        }
    }

    // Appliquer accélération
    Vector2 acceleration = Vector2Scale(pressureForce, 1.0f / density);
    Velocities[id] = Vector2Add(Velocities[id], Vector2Scale(acceleration, deltaTime));
}




void CalculateViscosity(int id)
{
    if (id >= numParticles) return;

    Vector2 pos = PredictedPositions[id];
    Vector2 originCell = GetCell2D(pos, smoothingRadius);
    float sqrRadius = smoothingRadius * smoothingRadius;

    Vector2 viscosityForce = Vector2Zero();
    Vector2 velocity = Velocities[id];

    // Parcours des 9 cellules voisines
    for (int i = 0; i < 9; i++)
    {
        int32_t hash = HashCell2D(Vector2Add(originCell, offsets2D[i]));
        int32_t key  = KeyFromHash(hash, numParticles);
        int32_t currIndex = SpatialOffsets[key];

        while (currIndex < numParticles)
        {
            int32_t neighbourIndex = currIndex;
            currIndex++;

            if (neighbourIndex == id) continue;

            int32_t neighbourKey = SpatialKeys[neighbourIndex];
            if (neighbourKey != key) break;

            Vector2 neighbourPos = PredictedPositions[neighbourIndex];
            Vector2 offsetToNeighbour = Vector2Subtract(neighbourPos, pos);
            float sqrDst = Vector2DotProduct(offsetToNeighbour, offsetToNeighbour);

            if (sqrDst > sqrRadius) continue;

            float dst = sqrtf(sqrDst);
            Vector2 neighbourVelocity = Velocities[neighbourIndex];

            // accumulation de la force de viscosité
            Vector2 diff = Vector2Subtract(neighbourVelocity, velocity);
            Vector2 force = Vector2Scale(diff, ViscosityKernel(dst, smoothingRadius));
            viscosityForce = Vector2Add(viscosityForce, force);
        }
    }

    Velocities[id] = Vector2Add(Velocities[id], Vector2Scale(viscosityForce, viscosityStrength * deltaTime));
}

void UpdatePositions(int id){
    if (id >= numParticles) return;
    Positions[id] = Vector2Add(Positions[id], Vector2Scale(Velocities[id], deltaTime));
    HandleCollisions(id);
}
