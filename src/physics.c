#include "visual.h"

#define SCALE_FACTOR 1e6 // facteur d'échelle pour les distances (1 unité = 1 million de mètres)


//--------------------- Gravity --------------------------

void rmviGravityRepulsion(rmviDynamic2D **features, int n){
    for(int i=0; i<n; i++){
        features[i]->force = (Vector2d){0};
        for(int j=0; j<n; j++){
            if(i!=j){
                Vector2d direction = Vector2dSubtract(features[j]->position, features[i]->position);
                double distance = Vector2dLength(direction);
                if(distance > 0){
                    Vector2d unit_direction = Vector2dScale(direction, 1.0/distance);
                    // Force gravitationnelle
                    Vector2d force_gravity = Vector2dScale(unit_direction, G * (features[i]->mass * features[j]->mass) / (distance * distance));
                    // Force de répulsion si trop proche
                    // Somme des forces
                    features[i]->force = Vector2dAdd(features[i]->force, force_gravity);
                }
            }
        }
    }
}

// autre équation à mettre ici pour avoir une meilleur vitesse
void rmviGravityRepulsion3D(rmviDynamic3D **features, int planetCount){
    for(int i=0; i<planetCount; i++){
        features[i]->force = (Vector3d) {0};
        for(int j=0; j<planetCount; j++){
            if(i!=j){
                Vector3d direction = Vector3dSubtract(features[j]->position, features[i]->position);
                double distance = Vector3dLength(direction);
                if(distance > 0){
                    Vector3d unit_direction = Vector3dScale(direction, 1.0/distance);
                    // Force gravitationnelle
                    Vector3d force_gravity = Vector3dScale(unit_direction, G * (features[i]->mass * features[j]->mass) / (distance * distance));
                    // Force de répulsion si trop proche
                    // Somme des forces
                    features[i]->force = Vector3dAdd(features[i]->force, force_gravity);
                }
            }
        }
    }
}

// math operation: // RMAPI ?
Vector2 Vector2d2Vector2(Vector2d v){
    return (Vector2) {(float) v.x,(float) v.y};
}
Vector2d Vector22Vector2d(Vector2 v){
    return (Vector2d) {(double) v.x,(double) v.y};
}

Vector2d Vector2dSubtract(Vector2d v1, Vector2d v2){
    Vector2d result = { v1.x - v2.x, v1.y - v2.y };

    return result;
}
double Vector2dLength(Vector2d v)
{
    double result = sqrtf((v.x*v.x) + (v.y*v.y));

    return result;
}
Vector2d Vector2dScale(Vector2d v, double scale){
    Vector2d result = { v.x*scale, v.y*scale };

    return result;
}
Vector2d Vector2dAdd(Vector2d v1, Vector2d v2){
    Vector2d result = { v1.x + v2.x, v1.y + v2.y };
    return result;
}

double Vector3dLength(const Vector3d v){
    double result = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    return result;
}
double Vector3dDistance(Vector3d v1, Vector3d v2){
    double result = 0.0f;
    double dx = v2.x - v1.x;
    double dy = v2.y - v1.y;
    double dz = v2.z - v1.z;
    result = sqrtf(dx*dx + dy*dy + dz*dz);

    return result;
}
Vector3d Vector3dNormalize(Vector3d v){
    Vector3d result = v;
    double length = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (length != 0.0f)
    {
        double ilength = 1.0f/length;
        result.x *= ilength;
        result.y *= ilength;
        result.z *= ilength;
    }

    return result;
}
Vector3d Vector3dRotateByAxisAngle(Vector3d v, Vector3d axis, double angle){
    Vector3d result = v;
    // Vector3Normalize(axis);
    double length = sqrtf(axis.x*axis.x + axis.y*axis.y + axis.z*axis.z);
    if (length == 0.0) length = 1.0;
    double ilength = 1.0/length;
    axis.x *= ilength;
    axis.y *= ilength;
    axis.z *= ilength;

    angle /= 2.0;
    double a = sinf(angle);
    double b = axis.x*a;
    double c = axis.y*a;
    double d = axis.z*a;
    a = cosf(angle);
    Vector3d w = { b, c, d };
    // Vector3CrossProduct(w, v)
    Vector3d wv = { w.y*v.z - w.z*v.y, w.z*v.x - w.x*v.z, w.x*v.y - w.y*v.x };
    // Vector3CrossProduct(w, wv)
    Vector3d wwv = { w.y*wv.z - w.z*wv.y, w.z*wv.x - w.x*wv.z, w.x*wv.y - w.y*wv.x };
    // Vector3Scale(wv, 2*a)
    a *= 2;
    wv.x *= a;
    wv.y *= a;
    wv.z *= a;
    // Vector3Scale(wwv, 2)
    wwv.x *= 2;
    wwv.y *= 2;
    wwv.z *= 2;
    result.x += wv.x;
    result.y += wv.y;
    result.z += wv.z;
    result.x += wwv.x;
    result.y += wwv.y;
    result.z += wwv.z;
    return result;
}

Vector3d Vector3dCrossProduct(Vector3d v1, Vector3d v2)
{
    Vector3d result = { v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x };

    return result;
}

Vector3d Vector3dScale(Vector3d v, double scalar){
    Vector3d result = { v.x*scalar, v.y*scalar, v.z*scalar };
    return result;
}
Vector3d Vector3dAdd(Vector3d v1, Vector3d v2){
    Vector3d result = { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
    return result;
}
Vector3d Vector3dSubtract(Vector3d v1, Vector3d v2){
    Vector3d result = { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
    return result;
}

Vector3 Vector3d2Vector3(Vector3d v){
    return (Vector3) {(float) v.x,(float) v.y,(float) v.z};
}
Vector3d Vector32Vector3d(Vector3 v){
    return (Vector3d) {(double) v.x,(double) v.y,(double) v.z};
}

// objet 3d suposé su un plan, on passe en polaire.
// on suppose 0 feature 




// ---------------------- Fluid Mechanichs


