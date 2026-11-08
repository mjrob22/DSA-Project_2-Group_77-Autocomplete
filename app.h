//
// app.h - The loaded program state shared by the terminal menu and the
// web app: both structures, their build stats, and the startup checks.
// Author: Navjot Singh
//

#ifndef DSA_PROJECT2_APP_H
#define DSA_PROJECT2_APP_H

#include <memory>
#include <string>
#include <vector>
#include "structure.h"
#include "dictionary.h"

struct Engine {
    std::unique_ptr<AutocompleteStructure> structure;
    BuildStats build;
    double avgQueryMs;
    std::string id;     // "trie" or "tst", used by the web app
    Engine(AutocompleteStructure* s, const std::string& engineId)
        : structure(s), avgQueryMs(0), id(engineId) {}
};

struct App {
    std::vector<Engine> engines;
    size_t current;
    std::string dictPath;
    size_t linesRead;
    double readMs;
    size_t agreementChecks;      // prefixes compared between the two structures
    size_t agreementMismatches;  // how many gave different results
    App() : current(0), linesRead(0), readMs(0), agreementChecks(0), agreementMismatches(0) {}

    AutocompleteStructure& active() { return *engines[current].structure; }
    bool hasBoth() const { return engines.size() >= 2; }

    const Engine* find(const std::string& engineId) const {
        for (size_t i = 0; i < engines.size(); i++)
            if (engines[i].id == engineId) return &engines[i];
        return nullptr;
    }
};

// Compares the two structures on every 1, 2 and 3 letter prefix
// (26 + 676 + 17,576 = 18,278 prefixes) and counts any that disagree.
const size_t AGREEMENT_K = 10;

inline void checkAgreement(App& app) {
    app.agreementChecks = 0;
    app.agreementMismatches = 0;
    if (!app.hasBoth()) return;
    const AutocompleteStructure& a = *app.engines[0].structure;
    const AutocompleteStructure& b = *app.engines[1].structure;
    std::string p;
    for (char c1 = 'a'; c1 <= 'z'; c1++) {
        for (int len2 = 0; len2 <= 26; len2++) {          // 0 = stop after one letter
            for (int len3 = 0; len3 <= 26; len3++) {
                if (len2 == 0 && len3 > 0) break;
                p.assign(1, c1);
                if (len2 > 0) p.push_back(static_cast<char>('a' + len2 - 1));
                if (len3 > 0) p.push_back(static_cast<char>('a' + len3 - 1));
                app.agreementChecks++;
                if (a.getSuggestions(p, AGREEMENT_K) != b.getSuggestions(p, AGREEMENT_K))
                    app.agreementMismatches++;
            }
        }
    }
}

#endif //DSA_PROJECT2_APP_H
