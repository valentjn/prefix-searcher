/* Copyright (C) 2021 Julian Valentin
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <omp.h>

#include "trie/Node.hpp"
#include "trie/Trie.hpp"

namespace trie {

Trie::Trie() : m_rootNode{std::make_unique<Node>()} {
}

Trie::Trie(const std::vector<std::string>& strings, size_t parallelPrefixLength)
      : m_rootNode{std::make_unique<Node>()} {
  if ((parallelPrefixLength == 0U) || (omp_get_max_threads() == 1)) {
    for (size_t stringIndex = 0U; stringIndex < strings.size(); stringIndex++) {
      insertString(strings, stringIndex);
    }
  } else {
    std::vector<std::string> bucketPrefixes;
    std::vector<std::vector<size_t>> buckets;
    std::vector<size_t> shortStringIndices;
    bucketSortStrings(strings, parallelPrefixLength, bucketPrefixes, buckets, shortStringIndices);

    std::vector<Trie> bucketTries{createBucketTries(strings, parallelPrefixLength, buckets)};

    for (size_t coarsePrefixLength = parallelPrefixLength; coarsePrefixLength-- > 0U;) {
      // reduce length of bucket prefixes by 1 by merging tries
      // (e.g., AB, AC, BD, BE: merge AB and AC tries to obtain an A trie, and
      // merge BD and BE tries to obtain a B trie)
      coarsenBucketTries(std::move(bucketPrefixes), std::move(bucketTries));
    }

    // bucketTries has size 1 at this point (all tries have merged into one)
    *this = std::move(bucketTries[0U]);

    // insert short strings
    for (const size_t& shortStringIndex : shortStringIndices) {
      insertString(strings, shortStringIndex);
    }
  }
}

Trie::Trie(
      const std::vector<std::string>& strings,
      const std::vector<size_t>& stringIndices,
      size_t ignorePrefixLength)
      : m_rootNode{std::make_unique<Node>()} {
  for (const size_t& stringIndex : stringIndices) {
    insertString(strings, stringIndex, ignorePrefixLength);
  }
}

Trie::Trie(
      std::vector<Trie>& tries,
      size_t trieBeginIndex,
      const std::vector<unsigned char>& keys)
      : m_rootNode{std::make_unique<Node>()} {
  // as child nodes for the new trie, use the root nodes of the tries with indices
  // trieBeginIndex, trieBeginIndex + 1, ..., trieBeginIndex + keys.size() - 1
  for (size_t i = 0U; i < keys.size(); i++) {
    m_rootNode->setChildNode(keys[i], std::move(tries[trieBeginIndex + i].m_rootNode));
  }
}

const Node& Trie::getRootNode() const {
  return *m_rootNode;
}

Node& Trie::getRootNode() {
  return *m_rootNode;
}

std::vector<size_t> Trie::searchPrefix(const std::string& prefix) const {
  const Node* descendantNode{m_rootNode->getDescendantNodeForPrefix(prefix)};
  std::vector<size_t> stringIndices;
  if (descendantNode != nullptr) descendantNode->collectStringIndices(stringIndices);
  return stringIndices;
}

void Trie::insertString(
      const std::vector<std::string>& strings,
      size_t stringIndex,
      size_t ignorePrefixLength) {
  Node* currentNode{m_rootNode.get()};

  for (size_t characterIndex = ignorePrefixLength; characterIndex < strings[stringIndex].size();
        characterIndex++) {
    const unsigned char byte{static_cast<unsigned char>(strings[stringIndex][characterIndex])};
    currentNode = &currentNode->getOrCreateChildNode(byte);
  }

  currentNode->setStringIndex(stringIndex);
}

void Trie::bucketSortStrings(
      const std::vector<std::string>& strings,
      size_t prefixLength,
      std::vector<std::string>& bucketPrefixes,
      std::vector<std::vector<size_t>>& buckets,
      std::vector<size_t> &shortStringIndices) {
  bucketPrefixes.clear();
  buckets.clear();
  shortStringIndices.clear();

  // powersOf256 will contain the powers of 256 in reverse order:
  // (256 ** (prefixLength - 1), 256 ** (prefixLength - 2), ..., 256, 1)
  std::vector<size_t> powersOf256(prefixLength);
  size_t currentPowerOf256 = 1U;

  for (size_t i = 0U; i < prefixLength; i++) {
    powersOf256[prefixLength - i - 1U] = currentPowerOf256;
    currentPowerOf256 *= 256U;
  }

  // currentPowerOf256 is 256 ** prefixLength at this point
  std::vector<std::vector<size_t>> allBucketVectors(currentPowerOf256);

  for (size_t stringIndex = 0U; stringIndex < strings.size(); stringIndex++) {
    if (strings[stringIndex].length() >= prefixLength) {
      // store indices of long strings in bucket corresponding to its prefix
      // bucket indices are in lexicographical order (e.g., AA, AB, BA, BB --> 0, 1, 2, 3)
      size_t bucketIndex = 0U;

      for (size_t i = 0U; i < prefixLength; i++) {
        bucketIndex += static_cast<unsigned char>(strings[stringIndex][i]) * powersOf256[i];
      }

      allBucketVectors[bucketIndex].push_back(stringIndex);
    } else {
      // store indices of short strings in shortStringIndices
      shortStringIndices.push_back(stringIndex);
    }
  }

  std::string currentPrefix(prefixLength, 'X');

  for (size_t bucketIndex = 0U; bucketIndex < allBucketVectors.size(); bucketIndex++) {
    // only include non-empty buckets in result
    if (!allBucketVectors[bucketIndex].empty()) {
      // recreate prefix from bucket indices
      for (size_t i = 0U; i < prefixLength; i++) {
        currentPrefix[i] = static_cast<char>((bucketIndex / powersOf256[i]) % 256U);
      }

      bucketPrefixes.push_back(currentPrefix);
      buckets.push_back(allBucketVectors[bucketIndex]);
    }
  }
}

std::vector<Trie> Trie::createBucketTries(
      const std::vector<std::string>& strings,
      const size_t prefixLength,
      const std::vector<std::vector<size_t>>& buckets) {
  std::vector<Trie> bucketTries(buckets.size());

  // create one trie for each bucket, ignoring the first prefixLength characters in each string
  #pragma omp parallel for default(none) shared(strings, prefixLength, buckets, bucketTries)
  for (size_t bucketIndex = 0U; bucketIndex < buckets.size(); bucketIndex++) {
    bucketTries[bucketIndex] = Trie(strings, buckets[bucketIndex], prefixLength);
  }

  return bucketTries;
}

void Trie::coarsenBucketTries(
      std::vector<std::string>&& bucketPrefixes,
      std::vector<Trie>&& bucketTries) {
  if (bucketPrefixes.empty()) return;
  // length of the prefix of the resulting buckets
  // (assumption: all strings in bucketPrefixes have the same length)
  const size_t coarsePrefixLength = bucketPrefixes[0U].length() - 1U;

  std::vector<std::string> coarseBucketPrefixes;
  std::vector<Trie> coarseTries;
  std::string coarseBucketPrefix(coarsePrefixLength, 'X');

  // coarseBucketBeginIndex is inclusive
  size_t coarseBucketBeginIndex = 0U;

  // buckets are contiguous, so it suffices to search coarseBucketEndIndex
  // starting from coarseBucketBeginIndex, which equals coarseBucketEndIndex
  // from the previous iteration
  // (assumption: bucketPrefixes is sorted)
  while (coarseBucketBeginIndex < bucketTries.size()) {
    bucketPrefixes[coarseBucketBeginIndex].copy(
        &coarseBucketPrefix[0U], coarsePrefixLength);

    // coarseBucketEndIndex is exclusive
    size_t coarseBucketEndIndex = coarseBucketBeginIndex + 1U;

    for (; coarseBucketEndIndex < bucketTries.size(); coarseBucketEndIndex++) {
      if (bucketPrefixes[coarseBucketEndIndex].compare(
            0U, coarsePrefixLength, coarseBucketPrefix) != 0) {
        break;
      }
    }

    // we know where the bucket starts and ends --> determine the keys of the child nodes
    // of the root node of the coarse trie to be created as the last character of each prefix
    // (the characters before are equal)
    std::vector<unsigned char> keys;

    for (size_t bucketIndex = coarseBucketBeginIndex; bucketIndex < coarseBucketEndIndex;
          bucketIndex++) {
      keys.push_back(static_cast<unsigned char>(
          bucketPrefixes[bucketIndex][coarsePrefixLength]));
    }

    // merge tries
    coarseBucketPrefixes.push_back(coarseBucketPrefix);
    coarseTries.emplace_back(bucketTries, coarseBucketBeginIndex, keys);
    coarseBucketBeginIndex = coarseBucketEndIndex;
  }

  bucketPrefixes = std::move(coarseBucketPrefixes);
  bucketTries = std::move(coarseTries);
}

}  // namespace trie
