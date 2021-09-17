/* Copyright (C) 2021 Julian Valentin
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TRIE_NODE_HPP
#define TRIE_NODE_HPP

#include <algorithm>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace trie {

class Node {
  public:
    static constexpr size_t INVALID_STRING_INDEX = std::numeric_limits<size_t>::max();

    Node() : m_stringIndex{INVALID_STRING_INDEX} {
    }

    bool getStringIndex() const {
      return m_stringIndex;
    }

    void setStringIndex(size_t stringIndex) {
      m_stringIndex = stringIndex;
    }

    size_t getSizeInMemory() const {
      return sizeof(Node) + m_keys.size() * sizeof(unsigned char)
          + m_childNodes.size() * sizeof(std::unique_ptr<Node>)
          + std::accumulate(std::begin(m_childNodes), std::end(m_childNodes), 0U,
            [](size_t sizeInMemory, const std::unique_ptr<Node>& childNode) {
              return sizeInMemory + childNode->getSizeInMemory();
            });
    }

    Node* getChildNode(unsigned char key) {
      const auto it = std::find(std::begin(m_keys), std::end(m_keys), key);
      return (it != std::end(m_keys)) ? m_childNodes[it - std::begin(m_keys)].get() : nullptr;
    }

    const Node* getChildNode(unsigned char key) const {
      const auto it = std::find(std::begin(m_keys), std::end(m_keys), key);
      return (it != std::end(m_keys)) ? m_childNodes[it - std::begin(m_keys)].get() : nullptr;
    }

    void setChildNode(unsigned char key, std::unique_ptr<Node> node) {
      const auto it = std::find(std::begin(m_keys), std::end(m_keys), key);

      if (it != std::end(m_keys)) {
        m_childNodes[it - std::begin(m_keys)] = std::move(node);
      } else {
        m_keys.push_back(key);
        m_childNodes.push_back(std::move(node));
      }
    }

    void collectStringIndices(std::vector<size_t>& stringIndices) const {
      if (m_stringIndex != INVALID_STRING_INDEX) stringIndices.push_back(m_stringIndex);

      for (size_t index = 0U; index < m_childNodes.size(); index++) {
        const std::unique_ptr<Node>& childNode{m_childNodes[index]};
        if (childNode) childNode->collectStringIndices(stringIndices);
      }
    }

  private:
    std::vector<unsigned char> m_keys;
    std::vector<std::unique_ptr<Node>> m_childNodes;
    size_t m_stringIndex;
};

}  // namespace trie

#endif  // #ifndef TRIE_NODE_HPP
