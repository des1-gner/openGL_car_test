#pragma once
#include "car.h"
#include "city.h"
#include <vector>

class Physics {
public:
    static void resolveCollisions(Car& car, const City& city);
    static void updateDrafting(std::vector<Car>& cars);
    static void updateAI(Car& aiCar, const Car& player, const City& city, float dt);
};
