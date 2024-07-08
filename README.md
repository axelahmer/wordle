# Wordle Analyzer

A C++ tool for analyzing Wordle puzzle guesses. It calculates the mean number of remaining solutions after each possible guess, helping players understand which words are most effective at narrowing down the solution space.

## Key Features
- Analyzes a `guess set` (14,855 words - all valid guesses allowed by Wordle) against a `solution set` (2,315 words - a subset of words that can be actual Wordle solutions)
- Uses up-to-date word lists from [alex1770's Wordle repository](https://github.com/alex1770/wordle)
- Optimized with multithreading and compile-time bitset computations

To modify these sets, update the corresponding header files.

Note: This tool provides data on guess effectiveness. It's not a solver, but its output can inform various solving strategies and make analyzing your Wordle games more insightful.

## Compile

Compile the program with the following command, using the -O4 optimization flag for best performance:

```
g++ -std=c++17 -static -O4 wordle.cpp -o wordle
```

## Run

Run the analyzer with optional guesses and their corresponding patterns:

```
./wordle [guess1] [pattern1] [guess2] [pattern2] ...
```

## Pattern Key

🟩 '=' : Correct letter, correct position
🟨 '+' : Correct letter, wrong position
⬛ '-' : Incorrect letter

## Usage Examples

### Example with Guesses

```
./wordle aloes +---+ scant =-=--
```

This represents:

A L O E S
🟨⬛⬛⬛🟨

S C A N T
🟩⬛🟩⬛⬛

Output:
```
Remaining possible solutions: 13
...

Top 10 best guesses:
krump: 1.31
phyma: 1.46
pharm: 1.46
prowk: 1.46
dhikr: 1.46
sharp: 1.62 *
smirk: 1.62
permy: 1.62
thrip: 1.62
thorp: 1.62

Top 10 worst guesses:
salas: 13.00
salal: 13.00
sajou: 13.00
beals: 13.00
sagos: 13.00
sages: 13.00
sagas: 13.00
safes: 13.00
saeta: 13.00
sabot: 13.00
...
```

The asterisk `*` indicates that the word is in the current set of possible solutions.

### Example with No Guesses

Running the analyzer without any guesses shows the overall effectiveness of words:

```
./wordle
```

Output:

| Top 10 Best Guesses | Score  | Top 10 Worst Guesses | Score   |
|---------------------|--------|----------------------|---------|
| naieo               | 148.89 | xviii                | 1988.54 |
| uraei               | 148.92 | zhuzh                | 1953.17 |
| aurei               | 149.47 | immix                | 1797.97 |
| ayrie               | 184.89 | susus                | 1780.85 |
| aiery               | 187.80 | jugum                | 1768.87 |
| aloes               | 191.71 | qajaq                | 1764.25 |
| heiau               | 192.30 | jujus                | 1762.42 |
| alose               | 193.10 | fuffy                | 1740.10 |
| ourie               | 195.30 | xylyl                | 1722.43 |
| olate               | 196.75 | yukky                | 1721.63 |

The score represents the mean number of remaining solutions after guessing that word. Lower scores indicate more effective guesses.