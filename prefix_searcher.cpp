/* Copyright (C) 2021 Julian Valentin
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <algorithm>
#include <chrono>
#include <functional>
#include <random>

#include "trie/Trie.hpp"

class Timer {
  public:
    void start() {
      m_begin = std::chrono::steady_clock::now();
    }

    void stop() {
      std::chrono::steady_clock::time_point end{std::chrono::steady_clock::now()};
      std::cout << "Finished in "
          << std::chrono::duration_cast<std::chrono::microseconds>(end - m_begin).count() / 1000.0
          << "ms." << std::endl;
    }

  private:
    std::chrono::steady_clock::time_point m_begin;
};

std::string generateRandomString(size_t length, std::function<char(void)> getRandomCharacter) {
  std::string string(length, 0U);
  std::generate_n(string.begin(), length, getRandomCharacter);
  return string;
}

std::vector<std::string> generateRandomStrings(
      size_t minimumLength, size_t maximumLength, size_t numberOfStrings) {
  const std::string characters{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
  const unsigned int seed{42U};

  std::default_random_engine randomNumberGenerator{seed};
  std::uniform_int_distribution<size_t> characterIndexDistribution{0U, characters.length() - 1U};
  std::function<char(void)> getRandomCharacter =
      [characters, &characterIndexDistribution, &randomNumberGenerator]() {
        return characters[characterIndexDistribution(randomNumberGenerator)];
      };

  std::uniform_int_distribution<size_t> lengthDistribution{minimumLength, maximumLength};
  std::vector<std::string> strings;

  for (size_t i = 0U; i < numberOfStrings; i++) {
    const size_t length{lengthDistribution(randomNumberGenerator)};
    strings.push_back(generateRandomString(length, getRandomCharacter));
  }

  return strings;
}

void testWithRandomStrings() {
  Timer timer;

  std::cout << "Generating random strings..." << std::endl;
  timer.start();
  std::vector<std::string> strings{generateRandomStrings(3U, 10U, 5000000U)};
  timer.stop();
  std::cout << std::endl;

  std::cout << "Constructing trie..." << std::endl;
  timer.start();
  trie::Trie stringTrie{strings};
  timer.stop();
  std::cout << "Number of nodes = " << stringTrie.getNumberOfNodes() << ", memory usage: "
      << stringTrie.getSizeInMemory() / (1024.0 * 1024.0) << " MiB" << std::endl << std::endl;

  const std::string fullPrefix{"abc"};

  for (size_t prefixLength = 1U; prefixLength <= fullPrefix.length(); prefixLength++) {
    std::cout << "Searching prefix of length " << prefixLength << "..." << std::endl;
    const std::string prefix{fullPrefix.substr(0U, prefixLength)};

    timer.start();
    const std::vector<size_t> stringIndices{stringTrie.searchPrefix(prefix)};
    timer.stop();
    std::cout << "Found " << stringIndices.size() << " match(es)." << std::endl << std::endl;
  }

  /*for (const size_t stringIndex : stringIndices) {
    std::cout << strings[stringIndex] << std::endl;
  }*/
}

int main() {
  const std::vector<std::string> strings{"wetter", "hallo", "hello", "welt", "world", "haus"};
  trie::Trie stringTrie{strings};

  std::cout << "Number of nodes = " << stringTrie.getNumberOfNodes() << ", memory usage: "
      << stringTrie.getSizeInMemory() / (1024.0 * 1024.0) << " MiB" << std::endl;

  const std::vector<size_t> stringIndices{stringTrie.searchPrefix("ha")};

  std::cout << "Found " << stringIndices.size() << " match(es):" << std::endl;

  for (const size_t stringIndex : stringIndices) {
    std::cout << strings[stringIndex] << std::endl;
  }

  testWithRandomStrings();

  return 0;
}
