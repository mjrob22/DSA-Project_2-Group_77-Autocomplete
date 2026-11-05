//
// Created by mjr12 on 11/2/2026.
//

#include <iostream>
#include <string>
#include <vector>
#include "trie.h"
using namespace std;

static int failures = 0;

// Prints a message and counts a failure if the condition is false
void check(bool condition, const string& description) {
    if (!condition) {
        cout << "FAIL: " << description << endl;
        failures++;
    }
}

int main() {
    Trie t;

    // Empty trie
    check(t.size() == 0, "empty trie has size 0");
    check(!t.contains("a"), "empty trie does not contain 'a'");
    check(t.getSuggestions("a", 5).empty(), "empty trie gives no suggestions");

    // insert/contains
    check(t.insert("apple"), "insert apple");
    check(t.insert("app"), "insert app");
    check(t.insert("apply"), "insert apply");
    check(t.insert("apt"), "insert apt");
    check(t.insert("banana"), "insert banana");
    check(t.size() == 5, "size is 5 after five inserts");
    check(t.contains("apple"), "contains apple");
    check(t.contains("app"), "contains app");
    check(!t.contains("ap"), "'ap' is only a prefix, not a word");
    check(!t.contains("apples"), "does not contain apples");
    check(!t.contains(""), "does not contain empty string");

    // duplicates, case handling, invalid input
    check(!t.insert("apple"), "duplicate insert returns false");
    check(!t.insert("APPLE"), "same word in uppercase is a duplicate");
    check(t.size() == 5, "size unchanged after duplicates");
    check(t.contains("Apple"), "contains is case-insensitive");
    check(!t.insert(""), "empty string rejected");
    check(!t.insert("don't"), "apostrophe rejected");
    check(!t.insert("abc1"), "digit rejected");
    check(!t.contains("abc"), "rejected word left no partial path");
    check(t.size() == 5, "size unchanged after invalid inserts");

    // suggestions: alphabetical, includes the prefix  if it is a word
    vector<string> s = t.getSuggestions("app", 10);
    check(s.size() == 3, "three suggestions for 'app'");
    if (s.size() == 3) {
        check(s[0] == "app", "first suggestion is app");
        check(s[1] == "apple", "second suggestion is apple");
        check(s[2] == "apply", "third suggestion is apply");
    }

    // limit k
    s = t.getSuggestions("a", 2);
    check(s.size() == 2, "k = 2 returns two suggestions");
    if (s.size() == 2) {
        check(s[0] == "app", "k = 2: first is app");
        check(s[1] == "apple", "k = 2: second is apple");
    }
    check(t.getSuggestions("a", 0).empty(), "k = 0 returns nothing");

    // no match, invalid prefix, uppercase prefix
    check(t.getSuggestions("z", 5).empty(), "no words start with z");
    check(t.getSuggestions("a1", 5).empty(), "invalid prefix returns nothing");
    check(t.getSuggestions("APP", 10).size() == 3, "uppercase prefix works");

    // empty prefix returns first k words
    s = t.getSuggestions("", 3);
    check(s.size() == 3, "empty prefix with k = 3 returns three words");
    if (s.size() == 3) {
        check(s[0] == "app", "empty prefix: first is app");
        check(s[1] == "apple", "empty prefix: second is apple");
        check(s[2] == "apply", "empty prefix: third is apply");
    }

    // node counting: root + a,p,p,l,e,y,t + b,a,n,a,n,a = 1 + 7 + 6 = 14
    check(t.nodeCount() == 14, "node count is 14");
    check(t.memoryBytes() > 0, "memory estimate is positive");

    // A longer word does not break a shorter word's end marker
    Trie u;
    u.insert("carton");
    check(!u.contains("car"), "car is not a word yet");
    u.insert("car");
    check(u.contains("car"), "car is a word after insert");
    check(u.contains("carton"), "carton is still a word");

    if (failures == 0) {
        cout << "All tests passed." << endl;
        return 0;
    }
    cout << failures << " test(s) failed." << endl;
    return 1;
}