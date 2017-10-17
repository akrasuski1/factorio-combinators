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

signal_t combine_signals(const signal_t& a, const signal_t& b) {
	signal_t out = a;
	for (const auto& kv: b) {
		out[kv.first] += kv.second;
	}
	std::vector<resource_t> todel;
	for (const auto& kv: out) {
		if (kv.second == 0){ 
			todel.push_back(kv.first);
		}
	}
	for (const auto res: todel) {
		out.erase(res);
	}
	return out;
}

void signal_inplace_subtract(signal_t& a, const signal_t& b) {
	for (auto kv: b) {
		a[kv.first] -= kv.second;
	}
}

void signal_inplace_add(signal_t& a, const signal_t& b) {
	for (auto kv: b) {
		a[kv.first] += kv.second;
	}
}
