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
  if ((parallelPrefixLength == 0) || (omp_get_max_threads() == 1)) {
    for (size_t stringIndex = 0U; stringIndex < strings.size(); stringIndex++) {
      insertString(strings, stringIndex);
    }
  } else {
    std::vector<std::string> bucketPrefixes;
    std::vector<std::vector<size_t>> buckets;
    std::vector<size_t> shortStringIndices;
    bucketSortStrings(strings, parallelPrefixLength, bucketPrefixes, buckets, shortStringIndices);

    std::vector<Trie> bucketTries{createBucketTries(strings, parallelPrefixLength, buckets)};

    for (size_t coarsePrefixLength = parallelPrefixLength; coarsePrefixLength-- > 0;) {
      coarsenBucketTries(bucketPrefixes, bucketTries);
    }

    *this = std::move(bucketTries[0]);

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

  std::vector<size_t> powersOf256(prefixLength);
  size_t currentPowerOf256 = 1U;

  for (size_t i = 0U; i < prefixLength; i++) {
    powersOf256[prefixLength - i - 1U] = currentPowerOf256;
    currentPowerOf256 *= 256U;
  }

  std::vector<std::vector<size_t>> allBucketVectors(currentPowerOf256);

  for (size_t stringIndex = 0U; stringIndex < strings.size(); stringIndex++) {
    if (strings[stringIndex].length() >= prefixLength) {
      size_t bucketIndex = 0U;

      for (size_t i = 0U; i < prefixLength; i++) {
        bucketIndex += static_cast<unsigned char>(strings[stringIndex][i]) * powersOf256[i];
      }

      allBucketVectors[bucketIndex].push_back(stringIndex);
    } else {
      shortStringIndices.push_back(stringIndex);
    }
  }

  std::string currentPrefix(prefixLength, 'X');

  for (size_t bucketIndex = 0U; bucketIndex < allBucketVectors.size(); bucketIndex++) {
    if (!allBucketVectors[bucketIndex].empty()) {
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

  #pragma omp parallel for default(none) shared(strings, prefixLength, buckets, bucketTries)
  for (size_t bucketIndex = 0U; bucketIndex < buckets.size(); bucketIndex++) {
    bucketTries[bucketIndex] = Trie(strings, buckets[bucketIndex], prefixLength);
  }

  return bucketTries;
}

void Trie::coarsenBucketTries(
      std::vector<std::string>& bucketPrefixes,
      std::vector<Trie>& bucketTries) {
  if (bucketPrefixes.empty()) return;
  const size_t coarsePrefixLength = bucketPrefixes[0].length() - 1;

  std::vector<std::string> coarseBucketPrefixes;
  std::vector<Trie> coarseTries;
  std::string coarseBucketPrefix(coarsePrefixLength, 'X');
  size_t coarseBucketBeginIndex = 0U;

  while (coarseBucketBeginIndex < bucketTries.size()) {
    bucketPrefixes[coarseBucketBeginIndex].copy(
        &coarseBucketPrefix[0], coarsePrefixLength);
    size_t coarseBucketEndIndex = coarseBucketBeginIndex + 1;

    for (; coarseBucketEndIndex < bucketTries.size(); coarseBucketEndIndex++) {
      if (bucketPrefixes[coarseBucketEndIndex].compare(
            0, coarsePrefixLength, coarseBucketPrefix) != 0) {
        break;
      }
    }

    std::vector<unsigned char> keys;

    for (size_t bucketIndex = coarseBucketBeginIndex; bucketIndex < coarseBucketEndIndex;
          bucketIndex++) {
      keys.push_back(static_cast<unsigned char>(
          bucketPrefixes[bucketIndex][coarsePrefixLength]));
    }

    coarseBucketPrefixes.push_back(coarseBucketPrefix);
    coarseTries.emplace_back(bucketTries, coarseBucketBeginIndex, keys);
    coarseBucketBeginIndex = coarseBucketEndIndex;
  }

  std::swap(bucketPrefixes, coarseBucketPrefixes);
  std::swap(bucketTries, coarseTries);
}

}  // namespace trie
