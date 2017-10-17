#include "simulation.h"

#include <iostream>

int main() {
	std::string blueprint_string;
	std::cin >> blueprint_string;
	Simulation simulation(blueprint_string);
	for (int i = 0; i < 100; i++) {
		std::cout << std::endl << "Tick #" << i << std::endl;
		simulation.tick();
	}
}
