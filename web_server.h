//
// web_server.h - Runs the Wordplay web app. A small local web server
// (cpp-httplib) serves web/index.html and answers the page's requests
// using the same Trie and TST objects the terminal menu uses.
//
//   GET /                 the page
//   GET /api/stats        build time, memory, nodes, average search, agreement check
//   GET /api/suggest      ?prefix=wond&k=6&mode=trie|tst|both
//
// Author: Navjot Singh
//

#ifndef DSA_PROJECT2_WEB_SERVER_H
#define DSA_PROJECT2_WEB_SERVER_H

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00      // cpp-httplib needs Windows 10 headers
#endif
#endif
#include "httplib.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "app.h"
#include "dictionary.h"

const int WEB_DEFAULT_PORT = 8080;
const int WEB_MAX_K = 50;
const size_t WEB_MAX_PREFIX = 40;

// ---------------------------------------------------------------- small JSON helpers

inline std::string jsonString(const std::string& s) {
    std::string out = "\"";
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (static_cast<unsigned char>(c) < 0x20) out += ' ';
        else out += c;
    }
    return out + "\"";
}

inline std::string jsonNumber(double v, int decimals = 3) {
    std::ostringstream os;
    os.setf(std::ios::fixed);
    os.precision(decimals);
    os << v;
    return os.str();
}

inline std::string jsonWords(const std::vector<std::string>& words) {
    std::string out = "[";
    for (size_t i = 0; i < words.size(); i++) {
        if (i) out += ",";
        out += jsonString(words[i]);
    }
    return out + "]";
}

// ---------------------------------------------------------------- the server

class WebServer {
public:
    explicit WebServer(App& app) : app_(app), port_(0) {}
    ~WebServer() { stop(); }

    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;

    // Starts listening in the background on the first free port from
    // `preferredPort` up. Returns the port, or 0 if none was free.
    int start(int preferredPort = WEB_DEFAULT_PORT) {
        if (running()) return port_;
        setupRoutes();
        // httplib's default (SO_REUSEPORT) lets two copies of the program share one
        // port. Use options that keep a port to one program, so a second copy
        // moves on to the next port instead.
        server_.set_socket_options([](socket_t sock) {
            int yes = 1;
#ifdef _WIN32
            setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&yes), sizeof(yes));
#else
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif
        });
        for (int p = preferredPort; p < preferredPort + 10; p++) {
            if (server_.bind_to_port("127.0.0.1", p)) { port_ = p; break; }
            server_.stop();   // a failed bind leaves the server unusable until stop() resets it
        }
        if (port_ == 0) return 0;
        thread_ = std::thread([this]() { server_.listen_after_bind(); });
        server_.wait_until_ready();
        return port_;
    }

    void stop() {
        if (thread_.joinable()) {
            server_.stop();
            thread_.join();
        }
    }

    bool running() const { return port_ != 0 && server_.is_running(); }
    int port() const { return port_; }
    std::string url() const { return "http://localhost:" + std::to_string(port_); }
    bool pageFound() const { return !findPage().empty(); }

    // Opens the page in the computer's default browser.
    void openBrowser() const {
        std::string u = url();
#if defined(_WIN32)
        std::string cmd = "start \"\" \"" + u + "\"";
#elif defined(__APPLE__)
        std::string cmd = "open \"" + u + "\"";
#else
        std::string cmd = "xdg-open \"" + u + "\" >/dev/null 2>&1 &";
#endif
        int ignored = std::system(cmd.c_str());
        (void)ignored;
    }

private:
    App& app_;
    httplib::Server server_;
    std::thread thread_;
    int port_;
    std::mutex countMutex_;
    std::map<std::string, size_t> countCache_;   // prefix -> how many words start with it

    // web/index.html, looked for next to the program and one or two folders up
    // (CLion and Visual Studio run the program from a build folder).
    static std::string findPage() {
        const char* places[] = {"web/index.html", "../web/index.html", "../../web/index.html"};
        for (size_t i = 0; i < sizeof(places) / sizeof(places[0]); i++) {
            std::ifstream f(places[i]);
            if (f) return places[i];
        }
        return "";
    }

    static std::string readFile(const std::string& path) {
        std::ifstream f(path.c_str(), std::ios::binary);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    // Lowercases the prefix. False if it has anything other than letters or is too long.
    static bool normalize(const std::string& raw, std::string& out) {
        out.clear();
        if (raw.size() > WEB_MAX_PREFIX) return false;
        for (size_t i = 0; i < raw.size(); i++) {
            char c = raw[i];
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
            if (c < 'a' || c > 'z') return false;
            out.push_back(c);
        }
        return true;
    }

    // Number of words starting with `prefix`, remembered after the first time.
    size_t countWords(const std::string& prefix) {
        {
            std::lock_guard<std::mutex> lock(countMutex_);
            std::map<std::string, size_t>::iterator it = countCache_.find(prefix);
            if (it != countCache_.end()) return it->second;
        }
        const AutocompleteStructure& s = *app_.engines.back().structure;
        size_t n = prefix.empty() ? s.size()
                                  : s.getSuggestions(prefix, static_cast<size_t>(-1)).size();
        std::lock_guard<std::mutex> lock(countMutex_);
        countCache_[prefix] = n;
        return n;
    }

    static void sendError(httplib::Response& res, int status, const std::string& message) {
        res.status = status;
        res.set_content("{\"error\":" + jsonString(message) + "}", "application/json");
    }

    void setupRoutes() {
        server_.Get("/", [this](const httplib::Request&, httplib::Response& res) { servePage(res); });
        server_.Get("/index.html", [this](const httplib::Request&, httplib::Response& res) { servePage(res); });
        server_.Get("/api/stats", [this](const httplib::Request&, httplib::Response& res) { serveStats(res); });
        server_.Get("/api/suggest", [this](const httplib::Request& req, httplib::Response& res) { serveSuggest(req, res); });
    }

    void servePage(httplib::Response& res) {
        std::string path = findPage();
        if (path.empty()) {
            res.status = 404;
            res.set_content("<h1>web/index.html not found</h1><p>Keep the web folder next to the program "
                            "(or one folder up from your build folder).</p>", "text/html; charset=utf-8");
            return;
        }
        res.set_header("Cache-Control", "no-store");
        res.set_content(readFile(path), "text/html; charset=utf-8");
    }

    void serveStats(httplib::Response& res) {
        std::string out = "{\"words\":" + std::to_string(app_.engines[0].structure->size()) +
                          ",\"agreement\":{\"checks\":" + std::to_string(app_.agreementChecks) +
                          ",\"mismatches\":" + std::to_string(app_.agreementMismatches) + "}" +
                          ",\"benchmark\":{\"prefixes\":" + std::to_string(BENCH_PREFIXES) +
                          ",\"k\":" + std::to_string(BENCH_K) + "}" +
                          ",\"structures\":[";
        for (size_t i = 0; i < app_.engines.size(); i++) {
            const Engine& e = app_.engines[i];
            const AutocompleteStructure& s = *e.structure;
            size_t nodes = s.nodeCount();
            if (i) out += ",";
            out += "{\"id\":" + jsonString(e.id) +
                   ",\"name\":" + jsonString(s.name()) +
                   ",\"buildMs\":" + jsonNumber(e.build.buildMs, 2) +
                   ",\"memoryBytes\":" + std::to_string(s.memoryBytes()) +
                   ",\"nodeCount\":" + std::to_string(nodes) +
                   ",\"nodeBytes\":" + std::to_string(nodes ? s.memoryBytes() / nodes : 0) +
                   ",\"avgQueryUs\":" + jsonNumber(e.avgQueryMs * 1000.0, 3) + "}";
        }
        out += "]}";
        res.set_header("Cache-Control", "no-store");
        res.set_content(out, "application/json");
    }

    void serveSuggest(const httplib::Request& req, httplib::Response& res) {
        std::string prefix;
        if (!normalize(req.get_param_value("prefix"), prefix))
            return sendError(res, 400, "Use letters a to z only, up to 40 of them.");

        int k = 6;
        std::string kText = req.get_param_value("k");
        if (!kText.empty()) {
            if (kText.size() > 3 || kText.find_first_not_of("0123456789") != std::string::npos)
                return sendError(res, 400, "k must be a number from 1 to 50.");
            k = std::atoi(kText.c_str());
        }
        if (k < 1 || k > WEB_MAX_K) return sendError(res, 400, "k must be a number from 1 to 50.");

        std::string mode = req.get_param_value("mode");
        if (mode.empty()) mode = "trie";
        std::vector<const Engine*> picked;
        if (mode == "both") {
            for (size_t i = 0; i < app_.engines.size(); i++) picked.push_back(&app_.engines[i]);
        } else {
            const Engine* e = app_.find(mode);
            if (!e) return sendError(res, 400, "mode must be trie, tst or both.");
            picked.push_back(e);
        }

        // results and timing for each structure asked for
        std::string results = "{";
        std::vector<std::vector<std::string> > lists;
        for (size_t i = 0; i < picked.size(); i++) {
            std::vector<std::string> words;
            double ms = timeQuery(*picked[i]->structure, prefix, static_cast<size_t>(k), words);
            lists.push_back(words);
            if (i) results += ",";
            results += jsonString(picked[i]->id) + ":{\"words\":" + jsonWords(words) +
                       ",\"us\":" + jsonNumber(ms * 1000.0, 3) + "}";
        }
        results += "}";
        bool match = true;
        for (size_t i = 1; i < lists.size(); i++) if (lists[i] != lists[0]) match = false;

        // the three most common next letters
        std::vector<std::pair<size_t, char> > next;
        if (!prefix.empty()) {
            for (char c = 'a'; c <= 'z'; c++) {
                size_t n = countWords(prefix + c);
                if (n > 0) next.push_back(std::make_pair(n, c));
            }
            std::sort(next.begin(), next.end(),
                      [](const std::pair<size_t, char>& x, const std::pair<size_t, char>& y) {
                          return x.first != y.first ? x.first > y.first : x.second < y.second;
                      });
            if (next.size() > 3) next.resize(3);
        }
        std::string nextJson = "[";
        for (size_t i = 0; i < next.size(); i++) {
            if (i) nextJson += ",";
            nextJson += "{\"letter\":\"" + std::string(1, next[i].second) + "\",\"count\":" +
                        std::to_string(next[i].first) + "}";
        }
        nextJson += "]";

        // in "both" mode: search time after each letter typed so far
        std::string steps = "[]";
        if (mode == "both" && !prefix.empty()) {
            steps = "[";
            for (size_t len = 1; len <= prefix.size(); len++) {
                std::string part = prefix.substr(0, len);
                if (len > 1) steps += ",";
                steps += "{\"prefix\":" + jsonString(part);
                for (size_t i = 0; i < picked.size(); i++) {
                    std::vector<std::string> ignored;
                    double ms = timeQuery(*picked[i]->structure, part, static_cast<size_t>(k), ignored);
                    steps += "," + jsonString(picked[i]->id) + ":" + jsonNumber(ms * 1000.0, 3);
                }
                steps += "}";
            }
            steps += "]";
        }

        const AutocompleteStructure& first = *app_.engines[0].structure;
        std::string out = "{\"prefix\":" + jsonString(prefix) +
                          ",\"k\":" + std::to_string(k) +
                          ",\"mode\":" + jsonString(mode) +
                          ",\"total\":" + std::to_string(countWords(prefix)) +
                          ",\"isWord\":" + std::string(first.contains(prefix) ? "true" : "false") +
                          ",\"match\":" + std::string(match ? "true" : "false") +
                          ",\"next\":" + nextJson +
                          ",\"results\":" + results +
                          ",\"steps\":" + steps + "}";
        res.set_header("Cache-Control", "no-store");
        res.set_content(out, "application/json");
    }
};

#endif //DSA_PROJECT2_WEB_SERVER_H
