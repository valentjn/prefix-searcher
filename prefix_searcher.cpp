/* Copyright (C) 2021 Julian Valentin
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <algorithm>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "trie/Trie.hpp"

class Timer {
  public:
    void start(const std::string& message) {
      std::cout << message << std::endl;
      m_begin = std::chrono::steady_clock::now();
    }

    void stop() {
      std::chrono::steady_clock::time_point end{std::chrono::steady_clock::now()};
      constexpr double numberOfMicrosecondsPerMillisecond = 1000.0;
      std::cout << "Finished in "
          << static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(
            end - m_begin).count()) / numberOfMicrosecondsPerMillisecond
          << "ms." << std::endl;
    }

  private:
    std::chrono::steady_clock::time_point m_begin;
};

void testSearchPrefix(
      const std::vector<std::string>& strings,
      const trie::Trie& trie,
      const std::string& prefix,
      bool printMatches = false) {
  Timer timer;
  timer.start("Searching prefix \"" + prefix + "\" via trie...");
  std::vector<size_t> stringIndices{trie.searchPrefix(prefix)};
  timer.stop();

  std::cout << "Found " << stringIndices.size() << " match";
  if (stringIndices.size() != 1U) {
    std::cout << "es";
  }

  if (printMatches) {
    std::cout << ":" << std::endl;
  } else {
    std::cout << "." << std::endl;
  }

  if (printMatches) {
    for (const size_t stringIndex : stringIndices) {
      std::cout << strings[stringIndex] << std::endl;
    }
  }

  std::cout << std::endl;
  std::vector<size_t> expectedStringIndices;
  timer.start("Searching prefix \"" + prefix + "\" via naive loop...");

  for (size_t stringIndex = 0U; stringIndex < strings.size(); stringIndex++) {
    if (strings[stringIndex].rfind(prefix, 0U) == 0U) {
      expectedStringIndices.push_back(stringIndex);
    }
  }

  timer.stop();
  std::sort(std::begin(stringIndices), std::end(stringIndices));

  if (stringIndices == expectedStringIndices) {
    std::cout << "Actual matches equal expected matches." << std::endl;
  } else {
    throw std::runtime_error("Actual matches do not equal expected matches.");
  }
}

void testWithSimpleExample() {
  std::cout << std::endl;

  const std::vector<std::string> strings{"wetter", "hallo", "hello", "welt", "world", "haus"};

  std::cout << "Input strings:" << std::endl;

  for (const std::string& string : strings) {
    std::cout << string << std::endl;
  }

  std::cout << std::endl;
  trie::Trie trie{strings};
  testSearchPrefix(strings, trie, "ha", true);
}

std::string generateRandomString(size_t length, std::function<char(void)> getRandomCharacter) {
  std::string string(length, 0U);
  std::generate_n(string.begin(), length, std::move(getRandomCharacter));
  return string;
}

std::vector<std::string> generateRandomStrings(
      size_t minimumLength,
      size_t maximumLength,
      size_t numberOfStrings) {
  const std::string characters{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
  const unsigned int seed{42U};

  std::default_random_engine randomNumberGenerator{seed};
  std::uniform_int_distribution<size_t> characterIndexDistribution{0U, characters.length() - 1U};
  std::function<char(void)> getRandomCharacter =
      [characters, &characterIndexDistribution, &randomNumberGenerator]() {
        return characters[characterIndexDistribution(randomNumberGenerator)];
      };

  std::uniform_int_distribution<size_t> lengthDistribution{minimumLength, maximumLength};
  std::set<std::string> stringSet;

  while (stringSet.size() < numberOfStrings) {
    const size_t length{lengthDistribution(randomNumberGenerator)};
    stringSet.insert(generateRandomString(length, getRandomCharacter));
  }

  return std::vector<std::string>(std::begin(stringSet), std::end(stringSet));
}

void testWithRandomStrings() {
  std::cout << std::endl;
  Timer timer;

  constexpr size_t minimumStringLength = 3U;
  constexpr size_t maximumStringLength = 30U;
  constexpr size_t numberOfStrings = 2000000U;
  timer.start("Generating random strings...");
  std::vector<std::string> strings{generateRandomStrings(
      minimumStringLength, maximumStringLength, numberOfStrings)};
  timer.stop();
  std::cout << std::endl;

  timer.start("Constructing trie...");
  trie::Trie trie{strings};
  timer.stop();

  constexpr double numberOfBytesPerMibibyte = 1024.0 * 1024.0;
  std::cout << "Memory usage: "
      << static_cast<double>(trie.getRootNode().getSizeInMemory()) / numberOfBytesPerMibibyte
      << " MiB" << std::endl;

  const std::string fullPrefix{"abcde"};

  for (size_t prefixLength = 1U; prefixLength <= fullPrefix.length(); prefixLength++) {
    const std::string prefix{fullPrefix.substr(0U, prefixLength)};
    std::cout << std::endl;
    testSearchPrefix(strings, trie, prefix);
  }
}

int main() {
  testWithSimpleExample();
  testWithRandomStrings();

  return 0;
}
