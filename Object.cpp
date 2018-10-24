#include "Object.h"

#include "header.h"


Material::Material():
    ambient(1.0f),//Default to white
    diffuse(1.0f),//Default to white
    specular(1.0f),//Default to white
    specularExponent(10.0f)//Used in lighting equation
{
}

//Constructor
//@transform The transformation matrix for the object
//@material The material properties of the object
Object::Object(const glm::mat4 &transform, const Material &material):
    _transform(transform),
    _material(material)
{
}

//Test whether a ray intersects the object
//@ray The ray that we are testing for intersection
//@info Object containing information on the intersection between the ray and the object(if any)
//virtual bool Intersect(const Ray &ray, IntersectInfo &info) const { return true; }
// bool Object::Intersect(const Ray &ray, IntersectInfo &info) const{
//   return true;
// }

// Helper method to safely solve a quadratic equation
// Adapted from resource at
// http://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
// Avoids catastrophic cancellation
bool solveQuadraticEquation( const float &a, const float &b, const float &c, float &root0, float &root1) {
    float discriminant = (b * b) - (4 * a * c);
    // Imaginary roots (a miss)
    if (discriminant < 0) {
        return false;
    }
    // repeated root - easy to calculate
    else if (discriminant == 0) {
        root0 = - 0.5 * b / a;
        root1 = - 0.5 * b / a;
    }
    // Two real distinct roots
    else {
        float q = (b > 0) ?
                  -0.5 * (b + sqrt(discriminant)) :
                  -0.5 * (b - sqrt(discriminant));
        root0 = q / a;
        root1 = c / q;
    }
    if (root0 > root1){
        // We want the smallest root first
        std::swap(root0, root1);
    }

    return true;
}

bool Sphere::Intersect(const Ray &ray, IntersectInfo &info) {

    // A ray can intersect a sphere 0, 1 or 2 times
    float root0, root1;

    // Vector of where the centre of the sphere from the origin of the ray
    glm::vec3 sphereOffset = (ray.origin - centre);

    // Compute the quadratic coefficients
    float a = glm::dot((ray.direction), (ray.direction)); //direction ^ 2
    float b = 2.0f * glm::dot((ray.direction), sphereOffset); 
    float c = glm::dot(sphereOffset, sphereOffset) - r_2;

    // Solve the quadratic equation at^2 + bt + c = 0
    if (!solveQuadraticEquation(a, b, c, root0, root1)) {
        return false;
    }

    if (root0 < 0) {
        root0 = root1; // If root 0 is negative then try root 1
        if (root0 < 0) {
            return false; // Both root 0 and root 1 are negative so no intersection
        }
    }

    // If we get here then a collison has occurred at t = root0
    info.time = root0;
    info.material = this->MaterialPtr();
    info.hitPoint = glm::vec3(ray(info.time));
    info.normal = glm::normalize(info.hitPoint - centre);
    return true;
}


bool Plane::Intersect(const Ray &ray, IntersectInfo &info) {

    // timeOfIntersect = (p0 - rayOrigin) . planeNormal
    //                      rayDirection . planeNormal
    float denominator = glm::dot(ray.direction, n);
    if (abs(denominator) < 1e-6) {
        // If denominator is close to 0 then the ray and the nornal are almost perpendicular so
        // the ray is almost parallel to the surface - thus it will miss
        return false;
    }
    else {
        glm::vec3 rayDist = (p0 - ray.origin);
        float timeOfIntersect = glm::dot(rayDist, n) / denominator;
        if (timeOfIntersect > 0) {
            info.time = timeOfIntersect;
            info.material = this->MaterialPtr();
            info.hitPoint = glm::vec3(ray(info.time));
            info.normal = glm::normalize(n);
            return true;
        }
    }

}

// The theory for this method was adapted from the course slides and the 'scratchapixel' online tutorial
// and the lecture slides
bool Triangle::Intersect(const Ray &ray, IntersectInfo &info) {
    // We need to decide if the ray intersects the plane that the triangle is on and then
    // if the intersection point is within the triangle boundaries

    // Does the ray intersect with the plane that the triangle is sitting on?
    glm::vec3 N = normal;
    glm::vec3 pA = A;
    glm::vec3 pB = B;
    glm::vec3 pC = C;

    // Need to check if the ray and plane are close to parallel - will be a miss
    float collisionDot = glm::dot(N, ray.direction);
    // The normal and the ray wil be perpendicular so the dot will be ~0
    if (abs(collisionDot) < 1e-6) {
        return false;
    }

    // Ray-triangle intersection from the slides
    // N . P + d = 0
    float numerator = -1.0f * glm::dot(N, (ray.origin - A));

    float timeOfIntersect = numerator / collisionDot;

    // Make sure that the 'collision' is not behind the ray
    if (timeOfIntersect < 0) {
        return false;
    }
    // P is the point of intersection
    glm::vec3 P = ray(timeOfIntersect);

    // Now we need to test if the pointOfIntersection lies within the given triangle ABC
    // The two barycentric edges u,v and the vector to intersect point w
    glm::vec3 u = B - A;
    glm::vec3 v = C - A;
    glm::vec3 w = P - A;

    float uu, uv, vv, wu, wv, denom;

    // Get the barycentric dot products
    uu = glm::dot(u, u);
    uv = glm::dot(u, v);
    vv = glm::dot(v, v);

    // Get the intersection point dot products
    wu = glm::dot(w, u);
    wv = glm::dot(w, v);

    // Shared denominator between s and t barycentric tests
    denom = (uv * uv) - (uu * vv);

    // Perform the barycentric tests
    float s, t;
    s = ((uv * wv) - (vv * wu)) / denom;
    if (s < 0.0 || s > 1.0) {         // outside triangle
        return false;
    }
    t = ((uv * wu) - (uu * wv)) / denom;
    if (t < 0.0 || (s + t) > 1.0) {   // outside triangle
        return false;
    }

    // If we get here then a collison has indeed occurred
    info.time = timeOfIntersect;
    info.material = this->MaterialPtr();
    info.hitPoint = glm::vec3(ray(info.time));
    info.normal = glm::normalize(N);
    return true;
}

bool AxisAlignedBox::Intersect(const Ray &ray, IntersectInfo &info) {

    // Get the data of the box
    float minX = min(p1.x, p2.x);
    float maxX = max(p1.x, p2.x);
    float minY = min(p1.y, p2.y);
    float maxY = max(p1.y, p2.y);
    float minZ = min(p1.z, p2.z);
    float maxZ = max(p1.z, p2.z);

    // Definitions for variables needed
    glm::vec3 N; // the normal of the face
    glm::vec3 P; // the point of intersection
    float denominator; // the ray direction . normal
    glm::vec3 rayDist;
    float timeOfIntersect;

    float delta = 1e-8;

    // 6 possible faces that could be collided with
    // Front face
    N = glm::vec3(0, 0, 1);
    denominator = glm::dot(ray.direction, N);
    // Fails if hitting from the underside or parallel
    if (abs(denominator) < delta) {
        //
    }
    else {
        rayDist = (glm::vec3((minX + maxX / 2.0f), (minY + maxY / 2.0f), maxZ) - ray.origin);
        timeOfIntersect = glm::dot(rayDist, N) / denominator;
        if (timeOfIntersect > 0) {
            P = ray(timeOfIntersect);
            if (P.x <= maxX && P.x >= minX &&
                    P.y <= maxY && P.y >= minY &&
                    P.z == maxZ) {

                if (timeOfIntersect < info.time) {
                    info.time = timeOfIntersect;
                    info.material = this->MaterialPtr();
                    info.hitPoint = glm::vec3(ray(info.time));
                    info.normal = glm::normalize(N);
                }
            }
        }
    }
    // Top face
    N = glm::vec3(0, 1, 0);
    denominator = glm::dot(ray.direction, N);
    // Fails if hitting from the underside or parallel
    if (abs(denominator) < delta) {
        //return false;
    }
    else {
        rayDist = (glm::vec3((minX + maxX / 2.0f), maxY, (minZ + maxZ / 2.0f)) - ray.origin);
        timeOfIntersect = glm::dot(rayDist, N) / denominator;
        if (timeOfIntersect > 0) {
            P = ray(timeOfIntersect);
            if (P.x <= maxX && P.x >= minX &&
                    P.y == maxY &&
                    P.z <= maxZ && P.z >= minZ) {

                if (timeOfIntersect < info.time) {
                    info.time = timeOfIntersect;
                    info.material = this->MaterialPtr();
                    info.hitPoint = glm::vec3(ray(info.time));
                    info.normal = glm::normalize(N);
                }
            }
        }
    }

    // Left face
    N = glm::vec3(-1, 0, 0);
    denominator = glm::dot(ray.direction, N);
    // Fails if hitting from the underside or parallel
    if (abs(denominator) < delta) {
        //return false;
    }
    else {
        rayDist = (glm::vec3(minX, (minY + maxY / 2.0f), (minZ + maxZ / 2.0f)) - ray.origin);
        timeOfIntersect = glm::dot(rayDist, N) / denominator;
        if (timeOfIntersect > 0) {
            P = ray(timeOfIntersect);
            if (P.x == minX &&
                    P.y <= maxY && P.y >= minY &&
                    P.z <= maxZ && P.z >= minZ) {

                if (timeOfIntersect < info.time) {
                    info.time = timeOfIntersect;
                    info.material = this->MaterialPtr();
                    info.hitPoint = glm::vec3(ray(info.time));
                    info.normal = glm::normalize(N);
                }
            }
        }
    }
    // Right face
    N = glm::vec3(1, 0, 0);
    denominator = glm::dot(ray.direction, N);
    // Fails if hitting from the underside or parallel
    if (abs(denominator) < delta) {
        //return false;
    }
    else {
        rayDist = (glm::vec3(maxX, (minY + maxY / 2.0f), (minZ + maxZ / 2.0f)) - ray.origin);
        timeOfIntersect = glm::dot(rayDist, N) / denominator;
        if (timeOfIntersect > 0) {
            P = ray(timeOfIntersect);
            if (P.x == maxX &&
                    P.y <= maxY && P.y >= minY &&
                    P.z <= maxZ && P.z >= minZ) {

                if (timeOfIntersect < info.time) {
                    info.time = timeOfIntersect;
                    info.material = this->MaterialPtr();
                    info.hitPoint = glm::vec3(ray(info.time));
                    info.normal = glm::normalize(N);
                }
            }
        }
    }

    // Back face
    N = glm::vec3(0, 0, -1);
    denominator = glm::dot(ray.direction, N);
    // Fails if hitting from the underside or parallel
    if (abs(denominator) < delta) {
        //
    }
    else {
        rayDist = (glm::vec3((minX + maxX / 2.0f), (minY + maxY / 2.0f), minZ) - ray.origin);
        timeOfIntersect = glm::dot(rayDist, N) / denominator;
        if (timeOfIntersect > 0) {
            P = ray(timeOfIntersect);
            if (P.x <= maxX && P.x >= minX &&
                    P.y <= maxY && P.y >= minY &&
                    P.z == minZ) {

                if (timeOfIntersect < info.time) {
                    info.time = timeOfIntersect;
                    info.material = this->MaterialPtr();
                    info.hitPoint = glm::vec3(ray(info.time));
                    info.normal = glm::normalize(N);
                }
            }
        }
    }
    // Bottom face
    N = glm::vec3(0, -1, 0);
    denominator = glm::dot(ray.direction, N);
    // Fails if hitting from the underside or parallel
    if (abs(denominator) < delta) {
        //return false;
    }
    else {
        rayDist = (glm::vec3((minX + maxX / 2.0f), minY, (minZ + maxZ / 2.0f)) - ray.origin);
        timeOfIntersect = glm::dot(rayDist, N) / denominator;
        if (timeOfIntersect > 0) {
            P = ray(timeOfIntersect);
            if (P.x <= maxX && P.x >= minX &&
                    P.y == minY &&
                    P.z <= maxZ && P.z >= minZ) {

                if (timeOfIntersect < info.time) {
                    info.time = timeOfIntersect;
                    info.material = this->MaterialPtr();
                    info.hitPoint = glm::vec3(ray(info.time));
                    info.normal = glm::normalize(N);
                }
            }
        }
    }

    if (info.time < std::numeric_limits<float>::infinity()) {
        return true;
    }
    // If we get here then fail
    return false;

}

float fmax(float f1, float f2, float f3) {
    float f = f1;

    if (f < f2) f = f2;
    if (f < f3) f = f3;

    return f;
}
