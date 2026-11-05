// Ternary Search Tree for the autocomplete engine

#ifndef DSA_PROJECT2_TST_H
#define DSA_PROJECT2_TST_H

#include <string>
#include <vector>
using namespace std;

class TST {
public:
    TST() : root_(nullptr), nodeCount_(0), wordCount_(0) {}
    ~TST() { destroy(root_); }

    // copying disabled
    TST(const TST&) = delete;
    TST& operator=(const TST&) = delete;

    // Inserts a word, turns all letters lowercase
    // return true if word was new and false if it was a duplicate, empty, or included a
    // non-alphabetic character
    bool insert(const string& word) {
        if (word.empty()) return false;
        // validate first 
        for (size_t i = 0; i < word.size(); i++) {
            if (toLower(word[i]) == 0) return false;
        }

        // link points at the pointer to the current node, starting with root_
        Node** link = &root_;
        size_t i = 0;
        char c = toLower(word[0]);
        while (true) {
            if (*link == nullptr) {
                *link = new Node(c);
                nodeCount_++;
            }
            Node* cur = *link;
            if (c < cur->ch) {
                link = &cur->lo;
            } else if (c > cur->ch) {
                link = &cur->hi;
            } else if (i + 1 < word.size()) {
                i++;
                c = toLower(word[i]);
                link = &cur->eq;
            } else {
                if (cur->isEnd) return false;
                cur->isEnd = true;
                wordCount_++;
                return true;
            }
        }
    }

    // True only if exact word was inserted, not just a prefix
    bool contains(const string& word) const {
        const Node* n = findNode(word);
        return n != nullptr && n->isEnd;
    }

    // Returns up to k words that start with prefix in alphabetical order
    // empty prefix returns the first k words in the whole tree
    vector<string> getSuggestions(const string& prefix, size_t k) const {
        vector<string> results;
        if (k == 0) return results;

        string current;
        current.reserve(prefix.size() + 16);

        // empty prefix: in-order traversal of the whole tree
        if (prefix.empty()) {
            collect(root_, current, k, results);
            return results;
        }

        const Node* start = findNode(prefix);
        if (start == nullptr) return results;

        for (size_t i = 0; i < prefix.size(); i++) {
            current.push_back(toLower(prefix[i]));
        }
        // prefix itself comes first alphabetically if it is a word
        if (start->isEnd) results.push_back(current);
        collect(start->eq, current, k, results);
        return results;
    }
    // # of distinct words
    size_t size() const {
        return wordCount_;
    }
    // nodes in the tree
    size_t nodeCount() const {
        return nodeCount_;
    }
    // node storage
    size_t memoryBytes() const {
        return nodeCount_ * sizeof(Node);
    }

private:
    struct Node {
        char ch;
        bool isEnd;
        Node* lo;   // characters less than ch
        Node* eq;   // next character in word
        Node* hi;   // characters greater than ch
        explicit Node(char c) : ch(c), isEnd(false), lo(nullptr), eq(nullptr), hi(nullptr) {}
    };

    Node* root_;
    size_t nodeCount_;
    size_t wordCount_;

    // Lowercases letter, return 0 if not a letter
    static char toLower(char c) {
        if (c >= 'a' && c <= 'z') return c;
        if (c >= 'A' && c <= 'Z') return static_cast<char>(c - 'A' + 'a');
        return 0;
    }

    // Finds the node for the last character of s, returns nullptr if the
    // path does not exist, s is empty, or s has an invalid character
    const Node* findNode(const string& s) const {
        if (s.empty()) return nullptr;
        const Node* cur = root_;
        size_t i = 0;
        char c = toLower(s[0]);
        if (c == 0) return nullptr;
        while (cur != nullptr) {
            if (c < cur->ch) {
                cur = cur->lo;
            } else if (c > cur->ch) {
                cur = cur->hi;
            } else {
                if (i + 1 == s.size()) return cur;
                i++;
                c = toLower(s[i]);
                if (c == 0) return nullptr;
                cur = cur->eq;
            }
        }
        return nullptr;
    }

    // In-order traversal- output alphabetical
    // Stops at k words
    void collect(const Node* node, string& current, size_t k,
                 vector<string>& out) const {
        if (node == nullptr || out.size() >= k) return;
        collect(node->lo, current, k, out);
        if (out.size() >= k) return;

        current.push_back(node->ch);
        if (node->isEnd) out.push_back(current);
        collect(node->eq, current, k, out);
        current.pop_back();

        collect(node->hi, current, k, out);
    }

    static void destroy(Node* node) {
        if (node == nullptr) return;
        destroy(node->lo);
        destroy(node->eq);
        destroy(node->hi);
        delete node;
    }
};
#endif 
