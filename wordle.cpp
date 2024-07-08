#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <bitset>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <queue>
#include <iomanip>
#include <thread>
#include <mutex>
#include <future>

#include "guess_words.h"
#include "solution_words.h"

constexpr size_t WORD_LENGTH = 5; // Length of each word
constexpr size_t MAX_WORDS = GUESS_COUNT; // Maximum number of words

using WordSet = std::bitset<MAX_WORDS>; // Bitset to keep track of possible words

class WordleAnalyzer {
private:
    // letter_positions[j][letter] tracks which words have `letter` at position `j`
    std::array<std::array<WordSet, 26>, WORD_LENGTH> letter_positions;

    // letter_presence[letter] tracks which words contain `letter` anywhere
    std::array<WordSet, 26> letter_presence;

    // Bitset of current possible solutions
    WordSet current_solutions;

    // Mutex for thread-safe access to results
    mutable std::mutex results_mutex;

    // Stores known guesses and patterns
    std::vector<std::pair<std::string, std::string>> known_guesses;

    // Preprocess the solution words to initialize letter positions and presence
    void preprocess_words() {
        for (size_t i = 0; i < SOLUTION_COUNT; ++i) {
            const auto& word = SOLUTION_WORDS[i];
            for (size_t j = 0; j < WORD_LENGTH; j++) {
                char letter = word[j] - 'a';
                letter_positions[j][letter].set(i);
                letter_presence[letter].set(i);
            }
        }
        current_solutions.set(); // All words are initially possible solutions
    }

    // Apply a guess to filter possible solutions
    WordSet apply_guess(const std::string& guess, const std::string& pattern) const {
        WordSet result = current_solutions;
        for (size_t i = 0; i < WORD_LENGTH; ++i) {
            char letter = guess[i] - 'a';
            if (pattern[i] == '=') { // Correct letter in correct position
                result &= letter_positions[i][letter];
            } else if (pattern[i] == '+') { // Correct letter in wrong position
                result &= letter_presence[letter] & ~letter_positions[i][letter];
            } else if (pattern[i] == '-') { // Incorrect letter
                result &= ~letter_presence[letter];
            }
        }
        return result;
    }

    // Calculate remaining words given a guess and solution
    WordSet calculate_remaining_words(const std::string& guess, const std::string& solution) const {
        WordSet remaining = current_solutions;
        for (size_t i = 0; i < WORD_LENGTH; ++i) {
            char g = guess[i] - 'a';
            char s = solution[i] - 'a';
            if (g == s) { // Correct letter in correct position
                remaining &= letter_positions[i][g];
            } else if (solution.find(guess[i]) != std::string::npos) { // Correct letter in wrong position
                remaining &= letter_presence[g] & ~letter_positions[i][g];
            } else { // Incorrect letter
                remaining &= ~letter_presence[g];
            }
        }
        return remaining;
    }

    // Analyze a single guess and update results
    void analyze_guess(size_t guess_index, std::vector<std::pair<std::string, double>>& results) const {
        const auto& guess = GUESS_WORDS[guess_index];
        double total_remaining = 0;
        for (size_t solution_index = 0; solution_index < SOLUTION_COUNT; ++solution_index) {
            if (current_solutions[solution_index]) {
                total_remaining += calculate_remaining_words(guess, SOLUTION_WORDS[solution_index]).count();
            }
        }
        double mean_remaining = total_remaining / current_solutions.count();
        {
            std::lock_guard<std::mutex> lock(results_mutex);
            results.emplace_back(guess, mean_remaining);
        }
    }

public:
    // Constructor to initialize and preprocess words
    WordleAnalyzer() {
        current_solutions.set();
        preprocess_words();
    }

    // Apply known guesses to filter possible solutions
    void apply_known_guesses(const std::vector<std::pair<std::string, std::string>>& guesses) {
        known_guesses = guesses;
        for (const auto& [guess, pattern] : guesses) {
            current_solutions &= apply_guess(guess, pattern);
        }
        std::cout << "Remaining possible solutions: " << current_solutions.count() << std::endl;
        if (current_solutions.count() == 1) {
            for (size_t i = 0; i < SOLUTION_COUNT; ++i) {
                if (current_solutions[i]) {
                    std::cout << "The solution is: " << SOLUTION_WORDS[i] << std::endl;
                    return;
                }
            }
        }
    }

    // Analyze all possible guesses to determine the best guess
    std::vector<std::pair<std::string, double>> analyze_possible_guesses() const {
        std::vector<std::pair<std::string, double>> results;
        results.reserve(GUESS_COUNT);

        std::vector<std::future<void>> futures;
        std::queue<std::chrono::high_resolution_clock::time_point> time_queue;

        auto start_time = std::chrono::high_resolution_clock::now();

        for (size_t guess_index = 0; guess_index < GUESS_COUNT; ++guess_index) {
            futures.push_back(std::async(std::launch::async, &WordleAnalyzer::analyze_guess, this, guess_index, std::ref(results)));

            // Performance logging
            auto current_time = std::chrono::high_resolution_clock::now();
            time_queue.push(current_time);
            if (time_queue.size() > 100) time_queue.pop();
            if (guess_index % 10 == 0 && time_queue.size() > 1) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - time_queue.front()).count();
                double words_per_second = 1000.0 * (time_queue.size() - 1) / duration;
                std::cout << "\rAnalyzed " << guess_index + 1 << "/" << GUESS_COUNT
                          << " guesses. Speed: " << std::fixed << std::setprecision(2)
                          << words_per_second << " words/s" << std::flush;
            }
        }

        for (auto& future : futures) {
            future.get();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "\nTime taken to analyze all guesses: " << duration.count() << " ms\n";

        return results;
    }

    // Check if a word matches known guesses in specified positions
    bool matches_known_guesses(const std::string& word) const {
        for (const auto& [guess, pattern] : known_guesses) {
            for (size_t i = 0; i < WORD_LENGTH; ++i) {
                if (pattern[i] == '=' && guess[i] != word[i]) {
                    return false;
                }
            }
        }
        return true;
    }

    // Getter for the number of remaining solutions
    size_t get_remaining_solutions_count() const {
        return current_solutions.count();
    }

    // Check if a word is in the current solutions set
    bool is_in_current_solutions(const std::string& word) const {
        for (size_t i = 0; i < SOLUTION_COUNT; ++i) {
            if (current_solutions[i] && SOLUTION_WORDS[i] == word) {
                return true;
            }
        }
        return false;
    }

    // Count matching letters with known guesses
    int count_matching_letters(const std::string& word) const {
        int match_count = 0;
        for (const auto& [guess, pattern] : known_guesses) {
            for (size_t i = 0; i < WORD_LENGTH; ++i) {
                if (pattern[i] == '=' && guess[i] == word[i]) {
                    match_count++;
                }
            }
        }
        return match_count;
    }
};

int main(int argc, char* argv[]) {
    auto start_time = std::chrono::high_resolution_clock::now();

    WordleAnalyzer analyzer;

    // Parse known guesses from command line arguments
    std::vector<std::pair<std::string, std::string>> known_guesses;
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 < argc) {
            known_guesses.emplace_back(argv[i], argv[i + 1]);
        }
    }
    analyzer.apply_known_guesses(known_guesses);

    // Analyze possible guesses if there are multiple solutions left
    if (analyzer.get_remaining_solutions_count() > 1) {
        auto results = analyzer.analyze_possible_guesses();

        // Sort results by mean remaining words, then by whether the word is in the current solutions set
        std::sort(results.begin(), results.end(),
                  [&analyzer](const auto& a, const auto& b) {
                      if (a.second != b.second) return a.second < b.second;
                      bool a_in_current_solutions = analyzer.is_in_current_solutions(a.first);
                      bool b_in_current_solutions = analyzer.is_in_current_solutions(b.first);
                      return a_in_current_solutions > b_in_current_solutions;
                  });

        // Output top 10 best guesses
        std::cout << "Top 10 best guesses:\n";
        for (size_t i = 0; i < 10 && i < results.size(); ++i) {
            bool in_current_solutions = analyzer.is_in_current_solutions(results[i].first);
            std::cout << results[i].first << ": " << std::fixed << std::setprecision(2) << results[i].second;
            if (in_current_solutions) {
                std::cout << " *";
            }
            std::cout << '\n';
        }

        // Output top 10 worst guesses
        std::cout << "\nTop 10 worst guesses:\n";
        for (size_t i = 0; i < 10 && i < results.size(); ++i) {
            bool in_current_solutions = analyzer.is_in_current_solutions(results[results.size() - 1 - i].first);
            std::cout << results[results.size() - 1 - i].first << ": " << std::fixed << std::setprecision(2) << results[results.size() - 1 - i].second;
            if (in_current_solutions) {
                std::cout << " *";
            }
            std::cout << '\n';
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "\nTotal time taken: " << duration.count() << " ms\n";

    return 0;
}
