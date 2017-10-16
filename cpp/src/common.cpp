#include "common.h"

#include "simulation.h"

#include <iostream>


void print_signal(std::ostream& os, const signal_t& sig, Simulation& simulation) {
	os << "{";
	for (const auto& kv: sig) {
		os << simulation.get_resource_name(kv.first) << ": " << kv.second << ", ";
	}
	os << "}";
}
