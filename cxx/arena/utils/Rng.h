#pragma once

#include <random>
#include <fstream>

class RNG {
public:    
    explicit RNG(uint64_t seed = std::random_device{}()) : gen(seed) {}
    void reseed(uint64_t seed) {gen.seed(seed);}
    
    void save(const std::string& path = "../data/rng_state.dat") {
        std::ofstream ofs(path, std::ios::binary);
        ofs << seed << "\n" << gen;
    }
    void load(const std::string& path = "../data/rng_state.dat") {
        std::ifstream ifs(path, std::ios::binary);
        ifs >> seed >> gen;
    }

    uint64_t getSeed() const {return seed;}

private:
    std::mt19937_64 gen;
    uint64_t seed;

};