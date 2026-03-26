#ifndef RMVIMATH_H
#define RMVIMATH_H
typedef struct Vector2d {
    double x;                // Vector x component
    double y;                // Vector y component
} Vector2d;

// Vector3, 3 components
typedef struct Vector3d {
    double x;                // Vector x component
    double y;                // Vector y component
    double z;                // Vector z component
} Vector3d;

// Vector4, 4 components
typedef struct Vector4d {
    double x;                // Vector x component
    double y;                // Vector y component
    double z;                // Vector z component
    double w;                // Vector w component
} Vector4d;

typedef struct {
    int *data;
    int count;
    int capacity;
} rmviIntList;

typedef struct{
    float *data;
    int count;
    int capacity;
} rmviFloatList;


rmviIntList rmviGetIntList(int num);
void rmviIntListInit(rmviIntList *list);
void rmviIntListFree(rmviIntList *list);
bool rmviIntListReserve(rmviIntList *list, int newCapacity);
bool rmviIntListAdd(rmviIntList *list, int value);
bool rmviIntListContains(const rmviIntList *list, int value);

void rmviFloatListInit(rmviFloatList *list);
rmviFloatList rmviGetFloatList(float num);
void rmviFloatListFree(rmviFloatList *list);
bool rmviFloatListReserve(rmviFloatList *list, int newCapacity);
bool rmviFloatListAdd(rmviFloatList *list, float value);
bool rmviFloatListContains(const rmviFloatList *list, float value);

Vector2 Vector2d2Vector2(Vector2d v);
Vector2d Vector22Vector2d(Vector2 v);

Vector2d Vector2dSubtract(Vector2d v1, Vector2d v2);
double Vector2dLength(Vector2d v);
Vector2d Vector2dScale(Vector2d v, double scale);
Vector2d Vector2dAdd(Vector2d v1, Vector2d v2);

double Vector3dLength(const Vector3d v);
double Vector3dDistance(Vector3d v1, Vector3d v2);
Vector3d Vector3dNormalize(Vector3d v);
Vector3d Vector3dRotateByAxisAngle(Vector3d v, Vector3d axis, double angle);

Vector3d Vector3dCrossProduct(Vector3d v1, Vector3d v2);
Vector3d Vector3dScale(Vector3d v, double scalar);
Vector3d Vector3dAdd(Vector3d v1, Vector3d v2);
Vector3d Vector3dSubtract(Vector3d v1, Vector3d v2);

Vector3 Vector3d2Vector3(Vector3d v);
Vector3d Vector32Vector3d(Vector3 v);
#endif