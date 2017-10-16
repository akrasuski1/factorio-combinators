#ifndef ENTITY_H_
#define ENTITY_H_

#include "simulation.h"

#include "lib/json11.hpp"

#include <iostream>


class Decider {
public:
	Decider(Simulation& simulation, json11::Json json);
	virtual void print(std::ostream& os);
private:
	resource_t left, right;
	int32_t constant;
	std::function<bool(int32_t, int32_t)> comparator;
	Simulation& simulation;
	std::string oper;
};

class Entity {
public:
	Entity(Simulation& simulation, json11::Json json);
	virtual void update();
	virtual void print(std::ostream& os);
	std::map<std::pair<int, Simulation::Color>,
	   	std::vector<std::pair<size_t, int>>> edges;
protected:
	Simulation& simulation;
	std::string name;
	std::unique_ptr<Decider> decider;
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
	resource_t left, right, output;
	int32_t constant;
	std::function<int32_t(int32_t, int32_t)> operation;
	std::string oper;
};

class DeciderCombinator: public Entity {
public:
	DeciderCombinator(Simulation& simulation, json11::Json json);
	virtual void print(std::ostream& os);
private:
	Decider decider;
	bool copy;
	resource_t output;
};

#endif
