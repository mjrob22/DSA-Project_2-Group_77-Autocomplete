// Benchmark for the Ternary Search Tree

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "tst.h"
using namespace std;

typedef chrono::steady_clock Clock;

static double msSince(Clock::time_point start) {
    return chrono::duration<double, milli>(Clock::now() - start).count();
}

static vector<string> loadWords(const string& path) {
    vector<string> words;
    ifstream in(path.c_str());
    string line;
    while (getline(in, line)) {
        // Strip a trailing carriage return from Windows line endings.
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        if (!line.empty()) words.push_back(line);
    }
    return words;
}

static void runBenchmark(const vector<string>& words, size_t n) {
    if (n > words.size()) n = words.size();

    // insert
    TST tst;
    Clock::time_point t0 = Clock::now();
    for (size_t i = 0; i < n; i++) tst.insert(words[i]);
    double insertMs = msSince(t0);

    // lookup any word that was inserted
    size_t found = 0;
    t0 = Clock::now();
    for (size_t i = 0; i < n; i++) {
        if (tst.contains(words[i])) found++;
    }
    double lookupMs = msSince(t0);

    // Prefix queries: first 3 letters of every 10th word, top 10 results each
    size_t queries = 0, returned = 0;
    t0 = Clock::now();
    for (size_t i = 0; i < n; i += 10) {
        string prefix = words[i].substr(0, 3);
        returned += tst.getSuggestions(prefix, 10).size();
        queries++;
    }
    double queryMs = msSince(t0);

    cout << "--- " << n << " words read, " << tst.size() << " valid words stored ---\n";
    cout << "Insert total:        " << insertMs << " ms\n";
    cout << "Lookup total:        " << lookupMs << " ms (" << found << " found)\n";
    cout << "Prefix query avg:    " << (queries ? queryMs * 1000.0 / queries : 0.0)
         << " us per query (" << queries << " queries, "
         << returned << " suggestions)\n";
    cout << "Nodes:               " << tst.nodeCount() << "\n";
    cout << "Approx. node memory: " << tst.memoryBytes() / (1024.0 * 1024.0) << " MB\n\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <wordlist.txt>\n";
        return 1;
    }
    vector<string> words = loadWords(argv[1]);
    if (words.empty()) {
        cout << "Could not read any words from " << argv[1] << "\n";
        return 1;
    }
    cout << "Loaded " << words.size() << " lines from " << argv[1] << "\n\n";

    size_t sizes[] = {10000, 100000, 250000};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        if (sizes[i] < words.size()) runBenchmark(words, sizes[i]);
    }
    runBenchmark(words, words.size());
    return 0;
}
