#include "visual.h"

#define SCALE_FACTOR 1e6 // facteur d'échelle pour les distances (1 unité = 1 million de mètres)
#define G 6.67430e-11 // constante gravitationnelle en m^3 kg^-1 s^-2

//--------------------- Gravity --------------------------
void rmviGravityRepulsion(rmviDynamic2D **features, int n){
    for(int i=0; i<n; i++){
        features[i]->force = (Vector2){0,0};
        for(int j=0; j<n; j++){
            if(i!=j){
                Vector2 direction = Vector2Subtract(features[j]->position, features[i]->position);
                float distance = Vector2Length(direction);
                if(distance > 0){
                    Vector2 unit_direction = Vector2Scale(direction, 1.0f/distance);
                    // Force gravitationnelle
                    Vector2 force_gravity = Vector2Scale(unit_direction, G * (features[i]->mass * features[j]->mass) / (distance * distance));
                    // Force de répulsion si trop proche
                    // Somme des forces
                    features[i]->force = Vector2Add(features[i]->force, force_gravity);
                }
            }
        }
    }
}

// autre équation à mettre ici pour avoir une meilleur vitesse
void rmviGravityRepulsion3D(rmviDynamic3D **features, int planetCount){
    for(int i=0; i<planetCount; i++){
        features[i]->force = Vector3Zero();
        for(int j=0; j<planetCount; j++){
            if(i!=j){
                Vector3 direction = Vector3Subtract(features[j]->position, features[i]->position);
                float distance = Vector3Length(direction);
                if(distance > 0){
                    Vector3 unit_direction = Vector3Scale(direction, 1.0f/distance);
                    // Force gravitationnelle
                    Vector3 force_gravity = Vector3Scale(unit_direction, G * (features[i]->mass * features[j]->mass) / (distance * distance));
                    // Force de répulsion si trop proche
                    // Somme des forces
                    features[i]->force = Vector3Add(features[i]->force, force_gravity);
                }
            }
        }
    }
}





// ---------------------- Fluid Mechanichs


