#include "simulation.h"

#include <iostream>

int main() {
	std::string blueprint_string;
	std::cin >> blueprint_string;
	Simulation simulation(blueprint_string);
}
