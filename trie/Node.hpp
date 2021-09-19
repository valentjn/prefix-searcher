/* Copyright (C) 2021 Julian Valentin
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TRIE_NODE_HPP
#define TRIE_NODE_HPP

#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace trie {

class Node {
  protected:
    using KeyChildNodePair = std::pair<unsigned char, std::unique_ptr<Node>>;

  public:
    static constexpr size_t INVALID_STRING_INDEX = std::numeric_limits<size_t>::max();

    size_t getStringIndex() const {
      return m_stringIndex;
    }

    void setStringIndex(size_t stringIndex) {
      m_stringIndex = stringIndex;
    }

    size_t getSizeInMemory() const {
      // account for size of this, the size of the heap memory reserved by m_keysAndChildNodes,
      // and the size of the heap memory reserved by the child node unique_ptrs
      return sizeof(Node)
          + m_keysAndChildNodes.size() * sizeof(KeyChildNodePair)
          + std::accumulate(std::begin(m_keysAndChildNodes), std::end(m_keysAndChildNodes), 0U,
            [](size_t sizeInMemory,
                  const KeyChildNodePair& keyChildNodePair) {
              return sizeInMemory + keyChildNodePair.second->getSizeInMemory();
            });
    }

    const Node* getChildNode(unsigned char key) const {
      return getChildNodeInternal(key);
    }

    Node* getChildNode(unsigned char key) {
      return getChildNodeInternal(key);
    }

    Node& getOrCreateChildNode(unsigned char key) {
      Node* childNode{getChildNode(key)};

      if (childNode == nullptr) {
        std::unique_ptr<Node> childNodeUniquePtr{std::make_unique<Node>()};
        childNode = childNodeUniquePtr.get();
        setChildNode(key, std::move(childNodeUniquePtr));
      }

      return *childNode;
    }

    void setChildNode(unsigned char key, std::unique_ptr<Node> node) {
      const auto it = findKey(key);

      if (it != std::end(m_keysAndChildNodes)) {
        it->second = std::move(node);
      } else {
        m_keysAndChildNodes.emplace_back(key, std::move(node));
      }
    }

    const Node* getDescendantNodeForPrefix(const std::string& prefix) const {
      const Node* currentNode{this};

      for (const char& character : prefix) {
        const unsigned char byte{static_cast<unsigned char>(character)};
        currentNode = currentNode->getChildNode(byte);

        if (currentNode == nullptr) {
          return nullptr;
        }
      }

      return currentNode;
    }

    void print(size_t indentationLevel = 0U) const {
      std::cout << "Node" << std::endl;

      for (const KeyChildNodePair& keyChildNodePair : m_keysAndChildNodes) {
        for (size_t level = 0U; level < indentationLevel + 1U; level++) {
          std::cout << "  ";
        }

        std::cout << "'" << keyChildNodePair.first << "' ("
            << static_cast<size_t>(keyChildNodePair.first) << "): ";
        keyChildNodePair.second->print(indentationLevel + 1U);
        // we don't have to print a newline, as this is done by the leaves
      }
    }

    void collectStringIndices(std::vector<size_t>& stringIndices) const {
      if (m_stringIndex != INVALID_STRING_INDEX) {
        stringIndices.push_back(m_stringIndex);
      }

      for (const KeyChildNodePair& keyChildNodePair : m_keysAndChildNodes) {
        if (keyChildNodePair.second) {
          keyChildNodePair.second->collectStringIndices(stringIndices);
        }
      }
    }

  protected:
    Node* getChildNodeInternal(unsigned char key) const {
      const auto it = findKey(key);
      return (it != std::end(m_keysAndChildNodes)) ? it->second.get() : nullptr;
    }

    std::vector<KeyChildNodePair>::const_iterator findKey(unsigned char key) const {
      return std::find_if(std::begin(m_keysAndChildNodes), std::end(m_keysAndChildNodes),
          [key](const KeyChildNodePair& keyChildNodePair) {
            return keyChildNodePair.first == key;
          });
    }

    std::vector<KeyChildNodePair>::iterator findKey(unsigned char key) {
      return std::find_if(std::begin(m_keysAndChildNodes), std::end(m_keysAndChildNodes),
          [key](const KeyChildNodePair& keyChildNodePair) {
            return keyChildNodePair.first == key;
          });
    }

  private:
    std::vector<KeyChildNodePair> m_keysAndChildNodes;
    size_t m_stringIndex{INVALID_STRING_INDEX};
};

}  // namespace trie

#endif  // #ifndef TRIE_NODE_HPP
