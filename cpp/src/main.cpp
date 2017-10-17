#include "simulation.h"

#include <iostream>
#include <fstream>
#include <chrono>


void benchmark() {
	std::string blueprint_string;
	std::cin >> blueprint_string;
	Simulation simulation(blueprint_string);
	auto t0 = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 50000; i++) {
		simulation.tick();
	}
	auto t1 = std::chrono::high_resolution_clock::now();
	size_t ms = std::chrono::duration_cast
		<std::chrono::milliseconds>(t1 - t0).count();
	double ups = 50000.0 / ms * 1000;
	std::cout << "UPS: " << ups << std::endl;
}

int main() {
	benchmark();
}
