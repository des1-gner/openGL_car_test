#include "physics.h"
#include <cmath>

void Physics::resolveCollisions(Car& car, const City& city) {
    float radius = car.width * 0.6f;
    if (city.collides(car.position, radius)) {
        Vec3 pushed = city.pushOut(car.position, radius);
        car.position = pushed;

        // Kill velocity component into the wall
        Vec3 delta = pushed - car.position;
        if (delta.lengthSq() > 0.0001f) {
            Vec3 wallNormal = delta.normalized();
            float velIntoWall = car.velocity.dot(wallNormal);
            if (velIntoWall < 0) {
                car.velocity -= wallNormal * velIntoWall * 1.2f;
            }
        }
        car.speed *= 0.7f;
        car.angularVel *= 0.5f;
    }
}

void Physics::updateDrafting(std::vector<Car>& cars) {
    float draftDist = 7.0f;
    float draftAngleRad = 18.0f * DEG2RAD;

    for (size_t i = 0; i < cars.size(); i++) {
        cars[i].inDraft = false;
        cars[i].draftBoost = 0;

        for (size_t j = 0; j < cars.size(); j++) {
            if (i == j) continue;
            Vec3 toOther = cars[j].position - cars[i].position;
            float dist = toOther.length();
            if (dist > draftDist || dist < 0.5f) continue;

            Vec3 myFwd = cars[i].forward();
            Vec3 dir = toOther.normalized();
            float dot = myFwd.dot(dir);
            if (dot < std::cos(draftAngleRad)) continue;

            Vec3 otherFwd = cars[j].forward();
            float alignment = myFwd.dot(otherFwd);
            if (alignment < 0.7f) continue;

            float strength = (1.0f - dist / draftDist) * alignment;
            if (strength > cars[i].draftBoost) {
                cars[i].draftBoost = strength;
                cars[i].inDraft = true;
            }
        }
    }
}

void Physics::updateAI(Car& aiCar, const Car& player, const City& city, float dt) {
    CarInput ai;
    ai.accelerate = true;

    Vec3 ahead = aiCar.position + aiCar.forward() * 5.0f;
    bool willCollide = city.collides(ahead, aiCar.width * 1.2f);

    Vec3 fwd = aiCar.forward();
    float angle = 0.35f;
    Vec3 aheadLeft = aiCar.position + Vec3(
        fwd.x*std::cos(angle) - fwd.z*std::sin(angle), 0,
        fwd.x*std::sin(angle) + fwd.z*std::cos(angle)) * 5.0f;
    Vec3 aheadRight = aiCar.position + Vec3(
        fwd.x*std::cos(-angle) - fwd.z*std::sin(-angle), 0,
        fwd.x*std::sin(-angle) + fwd.z*std::cos(-angle)) * 5.0f;

    bool leftClear = !city.collides(aheadLeft, aiCar.width);
    bool rightClear = !city.collides(aheadRight, aiCar.width);

    if (willCollide) {
        ai.brake = true;
        ai.accelerate = aiCar.speed < 5.0f;
        if (leftClear && !rightClear) ai.turnLeft = true;
        else if (rightClear && !leftClear) ai.turnRight = true;
        else ai.turnLeft = true;
    }

    // Chase player / draft
    Vec3 toPlayer = player.position - aiCar.position;
    float distP = toPlayer.length();
    if (distP < 20.0f && distP > 3.0f && !willCollide) {
        Vec3 dirP = toPlayer.normalized();
        float cross = fwd.x * dirP.z - fwd.z * dirP.x;
        if (cross > 0.15f) ai.turnLeft = true;
        else if (cross < -0.15f) ai.turnRight = true;
    }

    // Occasional drift for fun
    if (aiCar.speed > 15.0f && (ai.turnLeft || ai.turnRight) && std::rand() % 100 < 3) {
        ai.handbrake = true;
    }

    aiCar.update(ai, dt);
}
