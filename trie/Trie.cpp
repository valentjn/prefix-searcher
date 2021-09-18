/* Copyright (C) 2021 Julian Valentin
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <omp.h>

#include "trie/Node.hpp"
#include "trie/Trie.hpp"

#include <iostream>

namespace trie {

Trie::Trie(const std::vector<std::string>& strings) : m_numberOfNodes{1U} {
  for (size_t stringIndex = 0U; stringIndex < strings.size(); stringIndex++) {
    insertString(strings, stringIndex);
  }
}

const Node* Trie::getChildNode(const Node& node, unsigned char key) const {
  return node.getChildNode(key);
}

Node* Trie::getOrCreateChildNode(Node& node, unsigned char key) {
  Node* childNode{node.getChildNode(key)};

  if (childNode == nullptr) {
    std::unique_ptr<Node> childNodeUniquePtr{std::make_unique<Node>()};
    childNode = childNodeUniquePtr.get();
    node.setChildNode(key, std::move(childNodeUniquePtr));
    m_numberOfNodes++;
  }

  return childNode;
}

void Trie::insertString(const std::vector<std::string>& strings, size_t stringIndex) {
  Node* currentNode{&m_rootNode};

  for (const char& character : strings[stringIndex]) {
    const unsigned char byte{static_cast<unsigned char>(character)};
    currentNode = getOrCreateChildNode(*currentNode, byte);
  }

  currentNode->setStringIndex(stringIndex);
}

size_t Trie::getNumberOfNodes() const {
  return m_numberOfNodes;
}

size_t Trie::getSizeInMemory() const {
  return sizeof(Trie) + m_rootNode.getSizeInMemory();
}

std::vector<size_t> Trie::searchPrefix(const std::string& prefix) const {
  const Node* descendantNode{m_rootNode.getDescendantNodeForPrefix(prefix)};
  std::vector<size_t> stringIndices;
  if (descendantNode != nullptr) descendantNode->collectStringIndices(stringIndices);

  return stringIndices;
}

}  // namespace trie
