//
// Created by mjr12 on 11/2/2026.
//

#ifndef DSA_PROJECT2_TRIE_H
#define DSA_PROJECT2_TRIE_H

#include <string>
#include <vector>
using namespace std;

class Trie {
public:
    Trie() : root_(new Node()), nodeCount_(1), wordCount_(0) {}
    ~Trie() { destroy(root_); }

    // The trie owns raw pointers, so copying is disabled.
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    // Inserts a word, turns all letters lowercase, returns true if the
    // word was new and false if it was a duplicate, empty, or included a
    // non-alphabetic character
    bool insert(const string& word) {
        if (word.empty()) return false;
        // validate
        for (size_t i = 0; i < word.size(); i++) {
            if (indexOf(word[i]) < 0) return false;
        }
        Node* cur = root_;
        for (size_t i = 0; i < word.size(); i++) {
            int idx = indexOf(word[i]);
            if (cur->child[idx] == nullptr) {
                cur->child[idx] = new Node();
                nodeCount_++;
            }
            cur = cur->child[idx];
        }
        if (cur->isEnd) return false;
        cur->isEnd = true;
        wordCount_++;
        return true;
    }

    // True only if the exact word was inserted, not just a prefix
    bool contains(const string& word) const {
        const Node* n = findNode(word);
        return n != nullptr && n->isEnd && !word.empty();
    }

    // Returns up to k words that start with the prefix in alphabetical order
    // empty prefix returns the first k words in the whole trie
    vector<string> getSuggestions(const string& prefix, size_t k) const {
        vector<string> results;
        if (k == 0) return results;
        const Node* start = findNode(prefix);
        if (start == nullptr) return results;

        string current;
        current.reserve(prefix.size() + 16);
        for (size_t i = 0; i < prefix.size(); i++) {
            current.push_back(static_cast<char>('a' + indexOf(prefix[i])));
        }
        collect(start, current, k, results);
        return results;
    }
    // number of distinct words
    size_t size() const {
        return wordCount_;
    }
    // nodes including the root
    size_t nodeCount() const {
        return nodeCount_;
    }
    // node storage only
    size_t memoryBytes() const {
        return nodeCount_ * sizeof(Node);
    }

private:
    static const int ALPHABET = 26;

    struct Node {
        Node* child[ALPHABET];
        bool isEnd;
        Node() : isEnd(false) {
            for (int i = 0; i < ALPHABET; i++) child[i] = nullptr;
        }
    };

    Node* root_;
    size_t nodeCount_;
    size_t wordCount_;

    // Maps a letter to 0-25, -1 if not a letter.
    static int indexOf(char c) {
        if (c >= 'a' && c <= 'z') return c - 'a';
        if (c >= 'A' && c <= 'Z') return c - 'A';
        return -1;
    }

    // Walks the trie along the given string, returns nullptr if the path DNE
    // or the string has an invalid character
    const Node* findNode(const string& s) const {
        const Node* cur = root_;
        for (size_t i = 0; i < s.size(); i++) {
            int idx = indexOf(s[i]);
            if (idx < 0) return nullptr;
            cur = cur->child[idx];
            if (cur == nullptr) return nullptr;
        }
        return cur;
    }

    // Depth-first traversal, visiting children a-z so output is alphabetical
    // Stops at k words
    void collect(const Node* node, string& current, size_t k,
                 vector<string>& out) const {
        if (out.size() >= k) return;
        if (node->isEnd) out.push_back(current);
        for (int i = 0; i < ALPHABET && out.size() < k; i++) {
            if (node->child[i] != nullptr) {
                current.push_back(static_cast<char>('a' + i));
                collect(node->child[i], current, k, out);
                current.pop_back();
            }
        }
    }

    static void destroy(Node* node) {
        if (node == nullptr) return;
        for (int i = 0; i < ALPHABET; i++) destroy(node->child[i]);
        delete node;
    }
};
#endif //DSA_PROJECT2_TRIE_H