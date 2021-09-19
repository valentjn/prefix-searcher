/* Copyright (C) 2021 Julian Valentin
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TRIE_TRIE_HPP
#define TRIE_TRIE_HPP

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "trie/Node.hpp"

namespace trie {

class Trie {
  public:
    Trie();
    Trie(const std::vector<std::string>& strings, size_t parallelPrefixLength = 2U);
    Trie(
        const std::vector<std::string>& strings,
        const std::vector<size_t>& stringIndices,
        size_t ignorePrefixLength = 0);
    Trie(
        std::vector<Trie>& tries,
        size_t trieBeginIndex,
        const std::vector<unsigned char>& keys);

    const Node& getRootNode() const;
    Node& getRootNode();

    std::vector<size_t> searchPrefix(const std::string& prefix) const;

    void insertString(
        const std::vector<std::string>& strings,
        size_t stringIndex,
        size_t ignorePrefixLength = 0);

    static void bucketSortStrings(
        const std::vector<std::string>& strings,
        size_t prefixLength,
        std::vector<std::string>& bucketPrefixes,
        std::vector<std::vector<size_t>>& buckets,
        std::vector<size_t> &shortStringIndices);

    static std::vector<Trie> createBucketTries(
        const std::vector<std::string>& strings,
        const size_t prefixLength,
        const std::vector<std::vector<size_t>>& buckets);

    static void coarsenBucketTries(
        std::vector<std::string>& bucketPrefixes,
        std::vector<Trie>& bucketTries);

  private:
    std::unique_ptr<Node> m_rootNode;
};

}  // namespace trie

#endif  // #ifndef TRIE_TRIE_HPP
