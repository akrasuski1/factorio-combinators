#ifndef ENTITY_H_
#define ENTITY_H_

#include "simulation.h"

#include "lib/json11.hpp"

#include <iostream>


class Entity {
public:
	Entity(Simulation& simulation, json11::Json json);
	virtual void update();
	virtual void print(std::ostream& os);
protected:
	Simulation& simulation;
	json11::Json json;
};

std::ostream& operator<<(std::ostream& os, Entity& e);


class ConstantCombinator: public Entity {
public:
	ConstantCombinator(Simulation& simulation, json11::Json json);
	virtual void print(std::ostream& os);
private:
	signal_t constants;
};


class ArithmeticCombinator: public Entity {
public:
	ArithmeticCombinator(Simulation& simulation, json11::Json json);
	virtual void print(std::ostream& os);
private:
	bool valid;
	resource_t left, right, output;
	int32_t constant;
	std::function<int32_t(int32_t, int32_t)> operation;
};

#endif
