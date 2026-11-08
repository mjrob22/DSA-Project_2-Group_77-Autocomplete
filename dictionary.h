//
// dictionary.h - Loads the dwyl words_alpha.txt word list, builds a
// structure from it while showing a loading bar, and benchmarks queries.
// Author: Navjot Singh
//

#ifndef DSA_PROJECT2_DICTIONARY_H
#define DSA_PROJECT2_DICTIONARY_H

#include <chrono>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include "structure.h"
#include "ui.h"

typedef std::chrono::steady_clock Clock;

inline double msSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// Reads one word per line. Handles Windows "\r\n" endings and skips blank lines.
// Returns false if the file can't be opened.
inline bool loadWordFile(const std::string& path, std::vector<std::string>& words) {
    std::ifstream in(path.c_str());
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line[line.size() - 1] == '\r' || line[line.size() - 1] == ' '))
            line.erase(line.size() - 1);
        if (!line.empty()) words.push_back(line);
    }
    return true;
}

// Tries the given path, then the same file one and two folders up
// (CLion/Visual Studio run the program from a build folder).
inline bool findAndLoad(const std::string& path, std::string& usedPath,
                        std::vector<std::string>& words) {
    const char* prefixes[] = {"", "../", "../../", "data/", "../data/"};
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++) {
        std::string candidate = std::string(prefixes[i]) + path;
        words.clear();
        if (loadWordFile(candidate, words)) {
            usedPath = candidate;
            return true;
        }
    }
    return false;
}

struct BuildStats {
    double buildMs;     // time spent inside insert() only
    size_t inserted;    // words that were accepted (new + valid)
    BuildStats() : buildMs(0), inserted(0) {}
};

// Inserts every word, updating the loading bar between chunks.
// Only the insert calls are timed, so drawing the bar doesn't affect the result.
inline BuildStats buildStructure(AutocompleteStructure& s, const std::vector<std::string>& words) {
    BuildStats stats;
    const size_t chunks = 50;
    size_t chunkSize = words.size() / chunks + 1;
    std::string label = "Building " + s.shortName();

    for (size_t start = 0; start < words.size(); start += chunkSize) {
        size_t end = start + chunkSize;
        if (end > words.size()) end = words.size();

        Clock::time_point t0 = Clock::now();
        for (size_t i = start; i < end; i++) {
            if (s.insert(words[i])) stats.inserted++;
        }
        stats.buildMs += msSince(t0);
        ui::progress(label, static_cast<double>(end) / words.size());
    }
    if (words.empty()) ui::progress(label, 1.0);
    std::cout << "  " << ui::formatMs(stats.buildMs) << "\n";
    return stats;
}

// Runs one prefix query several times and returns the average time in ms.
// The results of the query are written to `out`.
const int QUERY_REPEATS = 200;

inline double timeQuery(const AutocompleteStructure& s, const std::string& prefix,
                        size_t k, std::vector<std::string>& out) {
    out = s.getSuggestions(prefix, k);            // warm-up run, keeps the results
    size_t sink = 0;
    Clock::time_point t0 = Clock::now();
    for (int r = 0; r < QUERY_REPEATS; r++) sink += s.getSuggestions(prefix, k).size();
    double ms = msSince(t0) / QUERY_REPEATS;
    if (sink == static_cast<size_t>(-1)) out.clear();   // stops the compiler skipping the loop
    return ms;
}

// A fixed set of random prefixes (1-3 letters taken from real words) so both
// structures are benchmarked on exactly the same queries. Fixed seed = repeatable.
inline std::vector<std::string> makeBenchmarkPrefixes(const std::vector<std::string>& words,
                                                      size_t count) {
    std::vector<std::string> prefixes;
    if (words.empty()) return prefixes;
    std::mt19937 rng(77);
    std::uniform_int_distribution<size_t> pickWord(0, words.size() - 1);
    std::uniform_int_distribution<size_t> pickLen(1, 3);
    while (prefixes.size() < count) {
        const std::string& w = words[pickWord(rng)];
        size_t len = pickLen(rng);
        if (len > w.size()) len = w.size();
        prefixes.push_back(w.substr(0, len));
    }
    return prefixes;
}

const size_t BENCH_PREFIXES = 1000;
const size_t BENCH_K = 10;
const int BENCH_ROUNDS = 20;

// Average time of one getSuggestions() call over all benchmark prefixes,
// repeated BENCH_ROUNDS times so small differences aren't just noise.
inline double averageQueryMs(const AutocompleteStructure& s,
                             const std::vector<std::string>& prefixes) {
    if (prefixes.empty()) return 0;
    size_t sink = 0;
    Clock::time_point t0 = Clock::now();
    for (int round = 0; round < BENCH_ROUNDS; round++)
        for (size_t i = 0; i < prefixes.size(); i++)
            sink += s.getSuggestions(prefixes[i], BENCH_K).size();
    double ms = msSince(t0) / (static_cast<double>(prefixes.size()) * BENCH_ROUNDS);
    if (sink == static_cast<size_t>(-1)) ms = 0;
    return ms;
}

#endif //DSA_PROJECT2_DICTIONARY_H
