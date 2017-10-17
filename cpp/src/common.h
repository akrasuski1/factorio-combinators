#ifndef COMMON_H_
#define COMMON_H_

#include <cstdint>
#include <unordered_map>


typedef uint16_t resource_t;
typedef std::unordered_map<resource_t, int32_t> signal_t;

class Simulation;
void print_signal(std::ostream& os, const signal_t& sig, Simulation& simulation);
signal_t combine_signals(const signal_t& a, const signal_t& b);

#endif
