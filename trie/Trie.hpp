/* Copyright (C) 2021 Julian Valentin
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TRIE_TRIE_HPP
#define TRIE_TRIE_HPP

#include <string>
#include <vector>

#include "trie/Node.hpp"

namespace trie {

class Trie {
  public:
    Trie(const std::vector<std::string>& strings);

    size_t getNumberOfNodes() const;

    size_t getSizeInMemory() const;

    std::vector<size_t> searchPrefix(const std::string& prefix) const;

  protected:
    const Node* getChildNode(const Node& node, unsigned char byte) const;
    Node* getOrCreateChildNode(Node& node, unsigned char byte);

    void insertString(const std::vector<std::string>& strings, size_t stringIndex);

  private:
    Node m_rootNode;
    size_t m_numberOfNodes;
};

}  // namespace trie

#endif  // #ifndef TRIE_TRIE_HPP
