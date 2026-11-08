//
// main.cpp - Menu-driven interface for the Group 77 autocomplete engine.
// Loads the word list into both structures, then lets the user search,
// switch structures, compare them side by side, view a performance
// dashboard, try live type-ahead search, or open the Wordplay web app.
// Author: Navjot Singh
//

#include <cctype>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Headers that pull in Windows system headers come first: the web server
// (winsock2.h must come before windows.h) and the terminal helpers. trie.h and
// tst.h say "using namespace std;", which makes std::byte clash with the
// Windows "byte" type if those system headers are included after them.
#include "web_server.h"
#include "live_input.h"
#include "ui.h"
#include "structure.h"
#include "dictionary.h"
#include "app.h"
#include "trie.h"

// The TST is picked up automatically as soon as tst.h is in the project.
// It must be a class named TST with the same public methods
// as Trie: insert, contains, getSuggestions, size, nodeCount, memoryBytes.
#if defined(__has_include)
#  if __has_include("tst.h")
#    include "tst.h"
#    define HAVE_TST 1
#  endif
#endif

using namespace std;

const int DEFAULT_K = 5;
const int MAX_K = 50;
const size_t LIVE_K = 10;

// ------------------------------------------------------------------ helpers

static string upper(string s) {
    for (size_t i = 0; i < s.size(); i++) s[i] = static_cast<char>(toupper(static_cast<unsigned char>(s[i])));
    return s;
}

static string inQuotes(const string& s) { return "\"" + s + "\""; }

// Lowercases the prefix. Returns false if it has anything other than letters.
static bool normalizePrefix(const string& raw, string& out) {
    out.clear();
    string s = ui::trim(raw);
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (!isalpha(c)) return false;
        out.push_back(static_cast<char>(tolower(c)));
    }
    return true;
}

static string askPrefix() {
    while (true) {
        string raw = ui::readLine("Enter prefix: ");
        string prefix;
        if (normalizePrefix(raw, prefix)) return prefix;
        cout << ui::INDENT << ui::red("Letters a-z only, please (no spaces, digits or symbols).") << "\n";
    }
}

static int askK() {
    return ui::readInt("Number of suggestions (k) [" + to_string(DEFAULT_K) + "]: ",
                       DEFAULT_K, 1, MAX_K);
}

// " 1. program" with the prefix highlighted, cut to fit `width` columns.
static string resultCell(size_t index, const string& word, size_t prefixLen, size_t width) {
    string num = (index < 9 ? " " : "") + to_string(index + 1) + ". ";
    size_t room = width > num.size() ? width - num.size() : 1;
    string shown = ui::truncate(word, room);
    size_t hl = prefixLen < shown.size() ? prefixLen : shown.size();
    return num + ui::highlightPrefix(shown, hl);
}

static void printWordCheck(const AutocompleteStructure& s, const string& prefix) {
    if (prefix.empty()) return;
    bool isWord = s.contains(prefix);
    cout << ui::INDENT << inQuotes(prefix) << " is a complete word: "
         << (isWord ? ui::green(string(ui::glyph().check) + " YES")
                    : ui::dim(string(ui::glyph().xmark) + " NO")) << "\n";
}

// ------------------------------------------------------------------ screen 1: menu

static int showMenu(App& app, const WebServer& web) {
    const ui::Glyphs& g = ui::glyph();
    ui::clearScreen();
    ui::boxTop();
    ui::boxLine(ui::bold("AUTOCOMPLETE ENGINE") + "  " + g.dot + "  Group 77", true);
    ui::boxLine(ui::dim("Dictionary loaded: " + ui::withCommas(app.engines[0].structure->size()) +
                        " words"), true);
    ui::boxDivider();
    ui::boxLine("[1] Search a prefix");

    string sw = ui::padRight("[2] Switch data structure", 34);
    if (app.hasBoth()) sw += string(g.arrow) + " current: " + ui::cyan(app.active().shortName());
    else sw += ui::dim("(TST not added yet)");
    ui::boxLine(sw);

    ui::boxLine(app.hasBoth() ? "[3] Compare Trie vs. TST on a prefix"
                              : "[3] Compare Trie vs. TST " + ui::dim("(needs TST)"));
    ui::boxLine("[4] View performance summary");
    ui::boxLine("[5] Live type-ahead mode");
    string webLine = ui::padRight("[6] Open the Wordplay web app", 34);
    if (web.running()) webLine += ui::green(string(g.arrow) + " localhost:" + to_string(web.port()));
    ui::boxLine(webLine);
    ui::boxLine("[7] Exit");
    ui::boxBottom();
    cout << "\n";
    return ui::readInt("Enter choice: ", -1, 1, 7);
}

// ------------------------------------------------------------------ screen 2: search

static void searchScreen(App& app) {
    AutocompleteStructure& s = app.active();
    ui::clearScreen();
    ui::titleBox("SEARCH  " + string(ui::glyph().dot) + "  " + s.name());
    cout << "\n";

    string prefix = askPrefix();
    int k = askK();

    vector<string> results;
    double ms = timeQuery(s, prefix, static_cast<size_t>(k), results);

    cout << "\n";
    if (results.empty()) {
        cout << ui::INDENT << ui::yellow("No words start with " + inQuotes(prefix) + ".") << "\n";
    } else {
        string what = prefix.empty() ? "the dictionary" : inQuotes(prefix);
        cout << ui::INDENT << ui::bold("Top " + to_string(results.size()) + " completions for " +
                                       what + " using " + s.name() + ":") << "\n\n";
        for (size_t i = 0; i < results.size(); i++)
            cout << ui::INDENT << resultCell(i, results[i], prefix.size(), 50) << "\n";
        if (results.size() < static_cast<size_t>(k))
            cout << "\n" << ui::INDENT << ui::dim("(only " + to_string(results.size()) +
                                                  " words start with this prefix)") << "\n";
    }
    cout << "\n";
    ui::rule();
    printWordCheck(s, prefix);
    cout << ui::INDENT << "Query time: " << ui::yellow(ui::formatMs(ms))
         << ui::dim("  (average of " + to_string(QUERY_REPEATS) + " runs)") << "\n";
    ui::pause();
}

// ------------------------------------------------------------------ screen 2b: switch

static void switchScreen(App& app) {
    if (!app.hasBoth()) {
        cout << "\n" << ui::INDENT
             << ui::yellow("Only the Trie is built right now. Add tst.h to the project to enable the TST.")
             << "\n";
        ui::pause();
        return;
    }
    app.current = (app.current + 1) % app.engines.size();
    // The menu shows the new "current" right away, so no extra screen is needed.
}

// ------------------------------------------------------------------ screen 3: compare

static void compareScreen(App& app) {
    const ui::Glyphs& g = ui::glyph();
    if (!app.hasBoth()) {
        cout << "\n" << ui::INDENT
             << ui::yellow("Comparison needs both structures. Add tst.h to the project first.") << "\n";
        ui::pause();
        return;
    }
    AutocompleteStructure& a = *app.engines[0].structure;
    AutocompleteStructure& b = *app.engines[1].structure;

    ui::clearScreen();
    ui::titleBox("COMPARE  " + string(g.dot) + "  " + a.name() + " vs. " + b.name());
    cout << "\n";
    string prefix = askPrefix();
    int k = askK();

    vector<string> ra, rb;
    double ta = timeQuery(a, prefix, static_cast<size_t>(k), ra);
    double tb = timeQuery(b, prefix, static_cast<size_t>(k), rb);
    bool match = (ra == rb);

    const size_t COL = 27;
    string sep = string(" ") + g.colSep + " ";

    cout << "\n" << ui::INDENT << "Prefix: " << ui::bold(prefix.empty() ? "(empty)" : prefix)
         << "      k = " << k << "\n\n";
    cout << ui::INDENT << ui::padRight(ui::bold(upper(a.name())), COL) << sep
         << ui::bold(upper(b.name())) << "\n";
    cout << ui::INDENT << ui::repeat(g.thin, COL + 1) << g.cross << ui::repeat(g.thin, COL + 1) << "\n";

    size_t rows = ra.size() > rb.size() ? ra.size() : rb.size();
    if (rows == 0)
        cout << ui::INDENT << ui::padRight(ui::dim("(no matches)"), COL) << sep
             << ui::dim("(no matches)") << "\n";
    for (size_t i = 0; i < rows; i++) {
        bool same = i < ra.size() && i < rb.size() && ra[i] == rb[i];
        string left = i < ra.size() ? resultCell(i, ra[i], prefix.size(), COL) : "";
        string right = i < rb.size() ? resultCell(i, rb[i], prefix.size(), COL) : "";
        if (!same) {   // a row that differs is shown in red on both sides
            left = i < ra.size() ? ui::red(ui::truncate(ra[i], COL)) : ui::red("(missing)");
            right = i < rb.size() ? ui::red(ui::truncate(rb[i], COL)) : ui::red("(missing)");
        }
        cout << ui::INDENT << ui::padRight(left, COL) << sep << right << "\n";
    }
    cout << ui::INDENT << ui::repeat(g.thin, COL + 1) << g.cross << ui::repeat(g.thin, COL + 1) << "\n";
    cout << ui::INDENT << ui::padRight("Query: " + ui::yellow(ui::formatMs(ta)), COL) << sep
         << "Query: " << ui::yellow(ui::formatMs(tb)) << "\n\n";

    double fast = ta < tb ? ta : tb, slow = ta < tb ? tb : ta;
    if (fast > 0 && slow / fast >= 1.05) {
        const string& winner = ta < tb ? a.name() : b.name();
        cout << ui::INDENT << "Faster:        " << ui::green(winner)
             << ui::dim("  (" + ui::formatRatio(slow / fast) + " as fast)") << "\n";
    } else {
        cout << ui::INDENT << "Faster:        " << ui::dim("about the same speed") << "\n";
    }
    cout << ui::INDENT << "Results match: "
         << (match ? ui::green(string(g.check) + " YES") : ui::red(string(g.xmark) + " NO")) << "\n";
    ui::pause();
}

// ------------------------------------------------------------------ screen 4: performance

// One metric row group: a bar per structure, the lowest value gets a green check.
static void metricBars(const App& app, const string& title, const vector<double>& values,
                       const vector<string>& labels, const string& bestWord) {
    const ui::Glyphs& g = ui::glyph();
    double maxV = 0, minV = values.empty() ? 0 : values[0];
    for (size_t i = 0; i < values.size(); i++) {
        if (values[i] > maxV) maxV = values[i];
        if (values[i] < minV) minV = values[i];
    }
    cout << ui::INDENT << ui::bold(title) << "\n";
    for (size_t i = 0; i < values.size(); i++) {
        string mark;
        if (values.size() > 1 && values[i] == minV) mark = ui::green(string(g.check) + " " + bestWord);
        cout << ui::INDENT << "  " << ui::padRight(app.engines[i].structure->shortName(), 6)
             << ui::bar(values[i], maxV, 24, i == 0 ? "36" : "35") << "  " << ui::padRight(labels[i], 12) << mark << "\n";
    }
    cout << "\n";
}

static void performanceScreen(App& app) {
    const ui::Glyphs& g = ui::glyph();
    ui::clearScreen();
    ui::titleBox("PERFORMANCE SUMMARY");
    cout << "\n";

    // --- number table
    const size_t LABEL = 20, VAL = 18;
    cout << ui::INDENT << ui::padRight("", LABEL);
    for (size_t i = 0; i < app.engines.size(); i++)
        cout << ui::padRight(ui::bold(app.engines[i].structure->shortName()), VAL);
    cout << "\n";
    ui::rule(LABEL + VAL * app.engines.size());

    vector<double> build, mem, query, nodes;
    vector<string> buildL, memL, queryL, nodesL, wordsL;
    for (size_t i = 0; i < app.engines.size(); i++) {
        const Engine& e = app.engines[i];
        build.push_back(e.build.buildMs);           buildL.push_back(ui::formatMs(e.build.buildMs));
        mem.push_back(static_cast<double>(e.structure->memoryBytes()));
        memL.push_back(ui::formatMB(e.structure->memoryBytes()));
        query.push_back(e.avgQueryMs);              queryL.push_back(ui::formatMs(e.avgQueryMs));
        nodes.push_back(static_cast<double>(e.structure->nodeCount()));
        nodesL.push_back(ui::withCommas(e.structure->nodeCount()));
        wordsL.push_back(ui::withCommas(e.structure->size()));
    }
    const vector<string>* rowsV[] = {&wordsL, &buildL, &memL, &queryL, &nodesL};
    const char* rowNames[] = {"Words stored", "Build time", "Memory used", "Avg query time", "Node count"};
    for (size_t r = 0; r < 5; r++) {
        cout << ui::INDENT << ui::padRight(rowNames[r], LABEL);
        for (size_t i = 0; i < rowsV[r]->size(); i++) cout << ui::padRight((*rowsV[r])[i], VAL);
        cout << "\n";
    }
    ui::rule(LABEL + VAL * app.engines.size());
    cout << "\n";

    // --- bar charts (shorter bar = better)
    metricBars(app, "Build time", build, buildL, "faster");
    metricBars(app, "Memory used", mem, memL, "smaller");
    metricBars(app, "Avg query time", query, queryL, "faster");
    metricBars(app, "Node count", nodes, nodesL, "fewer");

    // --- who won what
    if (app.hasBoth()) {
        vector<string> wins(app.engines.size());
        const vector<double>* metrics[] = {&build, &mem, &query};
        const char* names[] = {"build", "memory", "query"};
        for (size_t m = 0; m < 3; m++) {
            size_t best = (*metrics[m])[0] <= (*metrics[m])[1] ? 0 : 1;
            wins[best] += (wins[best].empty() ? "" : ", ") + string(names[m]);
        }
        cout << ui::INDENT << ui::bold("Wins: ");
        for (size_t i = 0; i < wins.size(); i++) {
            if (i) cout << "   " << g.dot << "   ";
            string nm = app.engines[i].structure->shortName();
            cout << (i == 0 ? ui::cyan(nm) : ui::magenta(nm)) << " "
                 << (wins[i].empty() ? ui::dim("none") : wins[i]);
        }
        cout << "\n\n";
    } else {
        cout << ui::INDENT << ui::dim("The TST column and comparison appear automatically once tst.h is added.")
             << "\n\n";
    }

    cout << ui::INDENT << ui::dim("Avg query = mean over " + ui::withCommas(BENCH_PREFIXES) +
                                  " random 1-3 letter prefixes x " + to_string(BENCH_ROUNDS) +
                                  " passes, k = " + to_string(BENCH_K) + ".") << "\n";
    cout << ui::INDENT << ui::dim("Memory = node storage reported by memoryBytes().") << "\n";
    cout << ui::INDENT << ui::dim("Word list read from disk in " + ui::formatMs(app.readMs) +
                                  " (" + ui::withCommas(app.linesRead) + " lines).") << "\n";
    ui::pause();
}

// ------------------------------------------------------------------ screen 5: live

static void liveScreen(App& app) {
    const ui::Glyphs& g = ui::glyph();
    if (!keyboardAvailable()) {
        cout << "\n" << ui::INDENT << ui::yellow("Live mode needs a real keyboard (input is coming from a file).")
             << "\n";
        ui::pause();
        return;
    }

    RawKeyboard keyboard;
    string prefix, notice;
    while (true) {
        AutocompleteStructure& s = app.active();
        vector<string> results;
        double ms = 0;
        if (!prefix.empty()) ms = timeQuery(s, prefix, LIVE_K, results);

        ui::clearScreen();
        ui::titleBox("LIVE TYPE-AHEAD  " + string(g.dot) + "  " + s.name());
        cout << ui::INDENT << ui::dim("Type to search  " + string(g.dot) + "  Tab = switch structure  " + g.dot +
                                      "  Esc = back") << "\n\n";

        ui::boxTop();
        ui::boxLine(ui::bold(prefix) + ui::cyan(g.cursor));
        ui::boxBottom();
        cout << "\n";

        if (prefix.empty()) {
            cout << ui::INDENT << ui::dim("Start typing to see suggestions...") << "\n";
        } else if (results.empty()) {
            cout << ui::INDENT << ui::yellow("No words start with " + inQuotes(prefix) + ".") << "\n";
        } else {
            for (size_t i = 0; i < results.size(); i++)
                cout << ui::INDENT << resultCell(i, results[i], prefix.size(), 50) << "\n";
        }
        cout << "\n";
        ui::rule();
        if (!prefix.empty()) {
            printWordCheck(s, prefix);
            cout << ui::INDENT << "Query time: " << ui::yellow(ui::formatMs(ms)) << "   "
                 << ui::dim(to_string(results.size()) + " shown") << "\n";
        }
        if (!notice.empty()) cout << ui::INDENT << ui::red(notice) << "\n";
        cout << flush;
        notice.clear();

        Key key = keyboard.read();
        if (key.type == KEY_ESCAPE || key.type == KEY_ENTER || key.type == KEY_END_OF_INPUT) break;
        if (key.type == KEY_BACKSPACE) {
            if (!prefix.empty()) prefix.erase(prefix.size() - 1);
        } else if (key.type == KEY_TAB) {
            if (app.hasBoth()) app.current = (app.current + 1) % app.engines.size();
            else notice = "Only the Trie is available right now.";
        } else if (key.type == KEY_CHAR) {
            unsigned char c = static_cast<unsigned char>(key.ch);
            if (!isalpha(c)) notice = "Letters a-z only.";
            else if (prefix.size() >= 40) notice = "That's long enough!";
            else prefix.push_back(static_cast<char>(tolower(c)));
        }
    }
}

// ------------------------------------------------------------------ startup

static bool loadEverything(App& app, const string& path) {
    cout << "\n" << ui::INDENT << ui::bold("AUTOCOMPLETE ENGINE  " + string(ui::glyph().dot) + "  Group 77")
         << "\n\n";

    vector<string> words;
    Clock::time_point t0 = Clock::now();
    if (!findAndLoad(path, app.dictPath, words) || words.empty()) {
        cout << ui::INDENT << ui::red("Could not read any words from " + inQuotes(path) + ".") << "\n"
             << ui::INDENT << "Put words_alpha.txt next to the program, or pass its path:\n"
             << ui::INDENT << "  ./autocomplete path/to/words_alpha.txt\n\n";
        return false;
    }
    app.readMs = msSince(t0);
    app.linesRead = words.size();
    cout << ui::INDENT << ui::padRight("Reading file", 16) << ui::withCommas(words.size())
         << " words from " << app.dictPath << "  (" << ui::formatMs(app.readMs) << ")\n";

    app.engines.push_back(Engine(new StructureAdapter<Trie>("Trie", "Trie"), "trie"));
#ifdef HAVE_TST
    app.engines.push_back(Engine(new StructureAdapter<TST>("Ternary Search Tree", "TST"), "tst"));
#endif

    for (size_t i = 0; i < app.engines.size(); i++)
        app.engines[i].build = buildStructure(*app.engines[i].structure, words);

    cout << ui::INDENT << ui::padRight("Benchmarking", 16) << ui::withCommas(BENCH_PREFIXES)
         << " random prefixes..." << flush;
    vector<string> prefixes = makeBenchmarkPrefixes(words, BENCH_PREFIXES);
    for (size_t i = 0; i < app.engines.size(); i++)
        app.engines[i].avgQueryMs = averageQueryMs(*app.engines[i].structure, prefixes);
    cout << " done\n";

    if (app.hasBoth()) {
        cout << ui::INDENT << ui::padRight("Checking", 16) << "Trie and TST agree on every 1-3 letter prefix..." << flush;
        checkAgreement(app);
        if (app.agreementMismatches == 0)
            cout << " " << ui::green(ui::withCommas(app.agreementChecks) + " of " +
                                     ui::withCommas(app.agreementChecks) + " match") << "\n";
        else
            cout << " " << ui::red(ui::withCommas(app.agreementMismatches) + " prefixes differ!") << "\n";
    }
    cout << "\n";

    // The structures hold their own copy of every word now, so free the list.
    vector<string>().swap(words);
    return true;
}

// ------------------------------------------------------------------ screen 6: web app

// Starts the web server in the background (if it isn't running yet) and
// opens the page. The menu keeps working while the web app is open.
static void webScreen(WebServer& web, bool openBrowser) {
    cout << "\n";
    if (!web.pageFound()) {
        cout << ui::INDENT << ui::red("Can't find web/index.html.") << "\n"
             << ui::INDENT << "Keep the web folder next to the program (or one folder up from your build folder).\n";
        return;
    }
    if (!web.running() && web.start() == 0) {
        cout << ui::INDENT << ui::red("Couldn't start the web app: ports 8080-8089 are all in use.") << "\n";
        return;
    }
    cout << ui::INDENT << ui::green("Wordplay is running at " + web.url()) << "\n"
         << ui::INDENT << ui::dim("If your browser doesn't open by itself, copy that address into it.") << "\n";
    if (openBrowser) web.openBrowser();
}

static void printUsage(const char* program) {
    cout << "Usage: " << program << " [path/to/words_alpha.txt] [--web] [--no-browser] [--ascii] [--no-color]\n"
         << "  --web         open the Wordplay web app instead of the terminal menu\n"
         << "  --no-browser  with --web, don't open a browser window automatically\n"
         << "  --ascii       use plain ASCII instead of box-drawing characters\n"
         << "  --no-color    turn off colors\n";
}

int main(int argc, char** argv) {
    ui::initTerminal();
    string path = "words_alpha.txt";
    bool webOnly = false, openBrowser = true;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--web") webOnly = true;
        else if (arg == "--no-browser") openBrowser = false;
        else if (arg == "--ascii") ui::settings().unicode = false;
        else if (arg == "--no-color") ui::settings().color = false;
        else if (arg == "--help" || arg == "-h") { printUsage(argv[0]); return 0; }
        else path = arg;
    }

    App app;
    WebServer web(app);
    try {
        if (!loadEverything(app, path)) return 1;

        if (webOnly) {
            webScreen(web, openBrowser);
            if (!web.running()) return 1;
            ui::readLine(ui::dim("Press Enter to stop the web app and quit..."));
            web.stop();
            cout << "\n" << ui::INDENT << "Goodbye!\n\n";
            return 0;
        }

        ui::readLine(ui::green("Ready!") + ui::dim("  Press Enter to open the menu..."));

        while (true) {
            int choice = showMenu(app, web);
            if (choice == 1) searchScreen(app);
            else if (choice == 2) switchScreen(app);
            else if (choice == 3) compareScreen(app);
            else if (choice == 4) performanceScreen(app);
            else if (choice == 5) liveScreen(app);
            else if (choice == 6) { webScreen(web, true); ui::pause(); }
            else if (choice == 7) break;
        }
    } catch (const ui::InputClosed&) {
        // input ended (Ctrl+D / Ctrl+Z): fall through to a clean exit
    }
    web.stop();
    cout << "\n" << ui::INDENT << "Goodbye!\n\n";
    return 0;
}
