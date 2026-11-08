//
// structure.h - One common interface for both autocomplete structures,
// so the menu code can switch between the Trie and the TST without
// caring which one it is talking to.
// Author: Navjot Singh
//

#ifndef DSA_PROJECT2_STRUCTURE_H
#define DSA_PROJECT2_STRUCTURE_H

#include <string>
#include <vector>

class AutocompleteStructure {
public:
    virtual ~AutocompleteStructure() {}

    virtual std::string name() const = 0;        // e.g. "Ternary Search Tree"
    virtual std::string shortName() const = 0;   // e.g. "TST"

    virtual bool insert(const std::string& word) = 0;
    virtual bool contains(const std::string& word) const = 0;
    virtual std::vector<std::string> getSuggestions(const std::string& prefix,
                                                    size_t k) const = 0;
    virtual size_t size() const = 0;         // distinct words stored
    virtual size_t nodeCount() const = 0;
    virtual size_t memoryBytes() const = 0;
};

// Wraps any class that has the same public methods as Trie
// (insert, contains, getSuggestions, size, nodeCount, memoryBytes).
template <typename T>
class StructureAdapter : public AutocompleteStructure {
public:
    StructureAdapter(const std::string& fullName, const std::string& shortName)
        : name_(fullName), short_(shortName) {}

    std::string name() const override { return name_; }
    std::string shortName() const override { return short_; }

    bool insert(const std::string& word) override { return impl_.insert(word); }
    bool contains(const std::string& word) const override { return impl_.contains(word); }
    std::vector<std::string> getSuggestions(const std::string& prefix,
                                            size_t k) const override {
        return impl_.getSuggestions(prefix, k);
    }
    size_t size() const override { return impl_.size(); }
    size_t nodeCount() const override { return impl_.nodeCount(); }
    size_t memoryBytes() const override { return impl_.memoryBytes(); }

private:
    T impl_;
    std::string name_;
    std::string short_;
};

#endif //DSA_PROJECT2_STRUCTURE_H
