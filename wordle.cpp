// wordle.cpp

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <intrin.h>

//TODO: use RMS of words left

const int MaxNumGuesses = 6;

const int MaxTargetCount = 2309 - 5;  // removed 5 recent solutions to make count % 64 == 0
const int MaxGuessCount = MaxTargetCount + 10688 + 64;

typedef char Word[8];      // could be 5 or 6 -- faster aligned
Word word[MaxGuessCount];  // target + extra guess words
int targetCount, guessCount;

void readWords() {
  FILE* fword = fopen("WordleWords.txt", "rt");
  while (1) {
    fscanf(fword, "%5s,", word[targetCount]);
    if (word[targetCount][0] == '!') break;
    ++targetCount;
  }

  guessCount = targetCount;
  while (1) {
    fscanf(fword, "%5s,", word[guessCount]);
    if (word[guessCount][0] == '!') break;
    ++guessCount;
  }
  fclose(fword);
}


char* response(const char* target, const char* guess) {
  static char resp[8];
  for (int pos = 5; --pos >= 0;)
    if (target[pos] == guess[pos]) resp[pos] = 'g';  // green
    else if (strchr(target, guess[pos])) resp[pos] = 'y'; // yellow
    else resp[pos] = ' ';

  return resp; // volatile!!
}

bool match(const char* target, const char* guess, const char* color) {
  for (int pos = 5; --pos >= 0;)
    switch (color[pos]) {
      case ' ': if (strchr(target, guess[pos])) return false; break; // grey
      case 'y': 
        if (!strchr(target, guess[pos])) return false;       // yellow
        if (target[pos] == guess[pos]) return false; break;
      case 'g': if (target[pos] != guess[pos]) return false; break;    // green
    }
  return true;
}

void matches() {
  for (int target = targetCount; --target >= 0;)
    if (match(word[target], "aurei", "   gy")
     && match(word[target], "islet", "y  gy")
    ) printf("%s\n", word[target]);
}


// find best guess words
 // by num words eliminated by guess (min total words remaining)

#if 0
  const char* guessed[8] = { "soare", };
#elif 1
  const char* guessed[8] = { "aurei", };
#else
const char* guessed[8] = { 0 };
#endif

char resp[8][8];

// TODO: see/use bestGuess

void bestWord(int n) { // 0.. 5
  int bestMatches = 99999999;
  int best;

  for (int guess = guessCount; --guess >= 0;) {
    int matches = 0;
    for (int target = targetCount; --target >= 0;) {
      for (int prev = n; --prev >= 0;)
        strcpy(resp[prev], response(word[target], guessed[prev]));
      strcpy(resp[n], response(word[target], word[guess]));

      for (int other = targetCount; --other >= 0;) {
        bool mismatch = false;
        for (int m = 0; m <= n; ++m)
          if (!match(word[other], word[target], resp[m])) {
            mismatch = true;
            break;
          }
        if (!mismatch) ++matches;
      }
    }
    if (matches < bestMatches) {
      bestMatches = matches;
      best = guess;
      printf("%s\n", word[best]); // aurei
    }
  } 
  guessed[n] = word[best];
}


void bestEliminator(int n) { // 0.. 5
  int maxEliminates = 0;
  int best = -1;

  for (int guess = guessCount; --guess >= 0;) {
    int eliminates = 0;
    for (int target = targetCount; --target >= 0;) {
      for (int prev = n; --prev >= 0;)
        strcpy(resp[prev], response(word[target], guessed[prev]));
      strcpy(resp[n], response(word[target], word[guess]));

      if (!strncmp(resp[n], "     ", 5)) continue; // don't eliminate the target!

      for (int other = targetCount; --other >= 0;) {
        for (int m = 0; m <= n; ++m)
          if (!match(word[other], word[target], resp[m])) {
            ++eliminates;
            break;
          }
      }
    }

    if (eliminates > maxEliminates) {
      maxEliminates = eliminates;
      best = guess;
      printf("%s %d\n", word[best], maxEliminates);
    }
  }
  guessed[n] = word[best];
}

bool eliminated[MaxTargetCount];

char* bestGuess() {  // given responses
  int maxEliminates = 0;
  int best = -1;

  int remain;
  int last = -1;

  for (int guess = guessCount; --guess >= 0;) {
    int eliminates = 0;
    remain = 0;
    for (int target = targetCount; --target >= 0;) {
      if (eliminated[target]) continue;
      ++remain;
      char* resp = response(word[target], word[guess]);
      if (!match(word[target], word[target], resp)) continue; // don't eliminate the target!
      for (int other = targetCount; --other >= 0;) {
        if (eliminated[other]) continue;
        if (!match(word[other], word[target], resp))
          ++eliminates;
        else last = other;
      }
    }

    if (eliminates > maxEliminates) {
      maxEliminates = eliminates;
      best = guess;
      printf("%s %d %d", word[best], maxEliminates, remain);
    }
  }

  if (best < 0) {    
    printf("%s %d\n", word[last], remain);
    exit(0);
    return word[last];
  }

  return word[best];
}

void bestGuesses() {
  char goal[8] = "swell";

  printf("%s\n", guessed[0]);

  for (int n = 0; n < 6; ++n) {
    if (guessed[n]) {
      strcpy(resp[n], response(goal, guessed[n]));
      for (int target = 0; target < targetCount; ++target)
        if (!match(word[target], guessed[n], resp[n]))
          eliminated[target] = true;
    }

    if (!guessed[n + 1])
      guessed[n + 1] = bestGuess();
    printf("\n");
  }
}

// hit <= 4: soare clint pudgy
// hit == 5, 6: roate sulci nymph
// hit >= 7: roate lysin chump   ****

void bestByWeights() { // more human useful responses
  const int match = 1, hit = 20;  // weights

  int bestGuess[4];

  for (int guessNum = 0; guessNum < 3; ++guessNum) {
    int best = 0;

    for (int guess = guessCount; --guess >= 0;) {
      bool dupl = false;
      for (int pos = 5; --pos >= 0;) {
        for (int prev = guessNum; --prev >= 0;)
          if (strchr(word[bestGuess[prev]], word[guess][pos])) {
            dupl = true;
            break;
          }
        if (dupl) break;
      }
      if (dupl) continue; // repeated guess letter unlikely useful

      int score = 0;
      for (int target = targetCount; --target >= 0;) {
        for (int pos = 5; --pos >= 0;) {
          if (word[guess][pos] == word[target][pos]) score += match;
          if (strchr(word[guess], word[target][pos])) score += hit;
        }
      }
      if (score > best) {
        best = score;
        bestGuess[guessNum] = guess;
      }
    }

    printf("%d: %5s\n", best, word[bestGuess[guessNum]]);
  }

  // frequency of remaining letters
  int count[5][26] = { 0 };
  for (int target = targetCount; --target >= 0;)
    for (int pos = 5; --pos >= 0;)
      ++count[pos][word[target][pos] - 'a'];

  char bestLetters[6] = "     ";

  for (int pos = 5; --pos >= 0;) {
    int best = 0;
    int bestPos, bestLetter;

    for (char letter = 'a'; letter <= 'z'; ++letter) {
      // if (strchr("wfxkz", letter)) continue;
      // if (strchr("vhjmq", letter)) continue;

      bool dupl = false;
      for (int pos = 5; --pos >= 0;) {
        if (strchr(bestLetters, letter)) {
          dupl = true;
          break;
        }

        for (int prev = 3; --prev >= 0;)
          if (strchr(word[bestGuess[prev]], letter)) {
            dupl = true;
            break;
          }
        if (dupl) break;
      }
      if (dupl) continue;

      for (int pos = 5; --pos >= 0;) {
        if (bestLetters[pos] == ' ')
          if (count[pos][letter - 'a'] > best) {
            best = count[pos][letter];
            bestPos = pos;
            bestLetter = letter;
          }
      }
    }
    bestLetters[bestPos] = bestLetter;
  }
  printf("%5s\n", bestLetters);
}


typedef unsigned __int64 Bits;

const int IntBits = sizeof(Bits) * 8;
const int BitVectLen = (MaxTargetCount + IntBits - 1) / IntBits;

typedef Bits Select[BitVectLen];
typedef Select Subset[26][5]; // [letter][pos]
Subset green, grnx, yelx, gryx;

int letterCount[26];

void initSubsets() { // bit set indicates excluded word
  // Bits lastVectMask = ((Bits)1 << (targetCount % IntBits)) - 1;  // not needed if targetCount % sizeof(Bits) == 0

  for (int t = targetCount; --t >= 0;) {
    for (int pos = 5; --pos >= 0;) {
      int letter = word[t][pos] - 'a';
      green[letter][pos][t / IntBits] |= (Bits)1 << (t % IntBits);
      ++letterCount[letter];
    }
  }

  for (int letter = 26; --letter >= 0;)
    for (int pos = 5; --pos >= 0;) {
      for (int i = BitVectLen; --i >= 0;)
        grnx[letter][pos][i] = ~green[letter][pos][i];
      // grnx[letter][pos][BitVectLen - 1] &= lastVectMask;
    }

  for (int letter = 26; --letter >= 0;)
    for (int pos = 5; --pos >= 0;) {
      for (int i = BitVectLen; --i >= 0;) {
        for (int opos = 5; --opos >= 0;)
          gryx[letter][opos][i] |= green[letter][pos][i];

        yelx[letter][pos][i] |= green[letter][pos][i]; // excluded at this position

        // yellow depends on union of other pos
        Select yellowLetter = { 0 };
        for (int q = 5; --q;) { // 4..1 offset
          int opos = (pos + q) % 5;
          yellowLetter[i] |= green[letter][opos][i];
        }
        yelx[letter][pos][i] |= ~yellowLetter[i];
        //if (i == BitVectLen - 1)
        //  yelx[letter][pos][BitVectLen - 1] &= lastVectMask;        
      }
    }
}

Bits letters[MaxGuessCount];
Bits LetterWeights[26];

void initLetterWeights() { // allocate 64 letters bits by letterCount weight 
  int wSum = 26;
  for (int l = 26; --l >= 0;) {
    LetterWeights[l] = (Bits)1 << l;
    int weight = (64 * letterCount[l] + (targetCount * 5 / 2 - 2450)) / (targetCount * 5) - 1;  // adjust rounding so that wSum == 64
    if (weight > 0) {
      LetterWeights[l] |= (((Bits)1 << weight) - 1) << wSum;
      wSum += weight;
    }
  }
  if (wSum > 64) exit(64);  // tune for 64
    
  for (int g = guessCount; --g >= 0;)
    for (int pos = 5; --pos >= 0;)
      letters[g] |= LetterWeights[word[g][pos] - 'a'];
}

void wordsLeft(Select& excluded) {
  for (int t = targetCount; --t >= 0;)
    if (!(excluded[t / IntBits] & ((Bits)1 << (t % IntBits))))
      printf("%s ", word[t]);
}

void remain() {
  char guess[6][8] = { "angel", "softy", "bicep" };
  char color[6][8] = { "y    ", "    y", "     " };

  Select excluded = { 0 };
  for (int g = 0; guess[g][0]; ++g)
    for (int pos = 5; --pos >= 0;) {
      Subset* exclude;
      switch (color[g][pos]) {
      case ' ':
      default:  exclude = &gryx; break;
      case 'y': exclude = &yelx; break;
      case 'g': exclude = &grnx; break;
      }
      for (int i = BitVectLen; --i >= 0;)
        excluded[i] |= (*exclude)[guess[g][pos] - 'a'][pos][i];
    }

  wordsLeft(excluded);
  printf("\n");

  // suggest best guess  TODO
}

double infoBits(Select& excluded, int target) {
  int count = 0;
  for (int i = BitVectLen; --i >= 0;)
    count += __popcnt64(excluded[i]);

  return log2(double(targetCount) / (targetCount - count));   // target in remaining count: 25% left = 2 bits of info
}

double calcInfo(const char** guess) {
  double info = 0;
  for (int t = targetCount; --t >= 0;) {
    Select excluded = { 0 };
    for (int pos = 5; --pos >= 0;)
      for (int n = 0; guess[n]; ++n) {
        int letter = guess[n][pos] - 'a';
        Select* bits;
        if (guess[n][pos] == word[t][pos])
          bits = &grnx[letter][pos];
        else if ((int)letters[t] & (1 << letter)) // match anywhere
          bits = &yelx[letter][pos];
        else bits = &gryx[letter][pos];

#if 0
        for (int i = BitVectLen; --i >= 0;)
          excluded[i] |= (*bits)[i];    // inner loop -- 37 * 64 long bit vector
#else // unrolled -> SSE instructions
        excluded[0] |= (*bits)[0];
        excluded[1] |= (*bits)[1];
        excluded[2] |= (*bits)[2];
        excluded[3] |= (*bits)[3];
        excluded[4] |= (*bits)[4];
        excluded[5] |= (*bits)[5];
        excluded[6] |= (*bits)[6];
        excluded[7] |= (*bits)[7];
        excluded[8] |= (*bits)[8];
        excluded[9] |= (*bits)[9];
        excluded[10] |= (*bits)[10];
        excluded[11] |= (*bits)[11];
        excluded[12] |= (*bits)[12];
        excluded[13] |= (*bits)[13];
        excluded[14] |= (*bits)[14];
        excluded[15] |= (*bits)[15];
        excluded[16] |= (*bits)[16];
        excluded[17] |= (*bits)[17];
        excluded[18] |= (*bits)[18];
        excluded[19] |= (*bits)[19];
        excluded[20] |= (*bits)[20];
        excluded[21] |= (*bits)[21];
        excluded[22] |= (*bits)[22];
        excluded[23] |= (*bits)[23];
        excluded[24] |= (*bits)[24];
        excluded[25] |= (*bits)[25];
        excluded[26] |= (*bits)[26];
        excluded[27] |= (*bits)[27];
        excluded[28] |= (*bits)[28];
        excluded[29] |= (*bits)[29];
        excluded[30] |= (*bits)[30];
        excluded[31] |= (*bits)[31];
        excluded[32] |= (*bits)[32];
        excluded[33] |= (*bits)[33];
        excluded[34] |= (*bits)[34];
        excluded[35] |= (*bits)[35];
#endif
      }
    info += infoBits(excluded, t);
  }
  return info / targetCount;  // average info revealed
}

void checkWords() {
  const char* words[][MaxNumGuesses] = {
    {"salet",0},
    {"soare","clint","pudgy",0},
    {"dance","blist","morph",0},
    {"robed","slant","chimp",0},
  };

  for (int n = 0; n < sizeof words / sizeof words[0]; ++n) {
    Bits coveredLetters = 0;
    const char* guesses[MaxNumGuesses] = { 0 };
    for (int w = 0; words[n][w]; ++w) {
      guesses[w] = (const char*)words[n][w];
      Bits thisLetters = 0;
      for (int pos = 5; --pos >= 0;)
        thisLetters |= LetterWeights[words[n][w][pos] - 'a'];
      coveredLetters |= thisLetters;
      printf("%s %.3f %2d  %.3f %2d\t", words[n][w], 
         calcInfo(&guesses[w]), (int)__popcnt64(thisLetters), 
         calcInfo(guesses), (int)__popcnt64(coveredLetters));
    }
    printf("%.4f\n", calcInfo(words[n]));
  }
  printf("\n");
}

double info[MaxGuessCount];
int rank[MaxGuessCount];

int rankByInfo(const void* arg1, const void* arg2) {
  return info[*(int*)arg2] > info[*(int*)arg1] ? 1 : -1;
}


void rankGuesses(int list = 5) { // single word rank
  const char* guesses[MaxNumGuesses] = { 0 };
  for (int guess = guessCount; --guess >= 0;) {
    guesses[0] = (const char*)word[guess];
    info[guess] = calcInfo(guesses);
  }

  for (int i = guessCount; --i >= 0;) rank[i] = i;
  qsort(&rank, guessCount, sizeof rank[0], rankByInfo);

  for (int i = 0; i < list; ++i)
    printf("%s %.3f %2d\n", word[rank[i]], info[rank[i]], (int)__popcnt64(letters[rank[i]]));
  printf("\n");
}


const int MaxNumPairs = 8192; //  MaxTargetCount * MaxTargetCount / N;  // out of 4M    1567
short topPair[MaxNumPairs][2];  // guess indices
float pairInfo[MaxNumPairs];
float minPairInfo = 6; // to 7.45 max

void addPair(int g0, int g1, float newPairInfo) {
  while (1) {
    for (int pair = 0; pair < MaxNumPairs; ++pair) {
      if (pairInfo[pair] <= minPairInfo) {
        pairInfo[pair] = newPairInfo;
        topPair[pair][0] = g0;
        topPair[pair][1] = g1;
        return;
      }
    }
    minPairInfo += minPairInfo / 100;
    printf("\t%.3f\r", minPairInfo);
  }
}

// salet 4.69
// artel 4.96
// aitch sedgy 7.45

// probe triples using ranked pairs list
//   keep only top N pairs -- min info threshold to store?

// simple trees based on probe results
//   vs. 2.7, 0.8 bits added using fixed triples


void bestInfoGuesses(int numGuesses = 2) {
  int guess[MaxNumGuesses];
  int best[MaxNumGuesses];
  Bits coveredLetters[MaxNumGuesses];

  const double InfoThresh[MaxNumGuesses] = { 0, 4, 9, 8, 10.7 };  // start progress display
  double bestInfo = InfoThresh[numGuesses];
  const char* guesses[MaxNumGuesses] = { 0 };

  for (int g0 = 0; g0 < guessCount/4; ++g0) {  // or less - best are at top
    guess[0] = rank[g0];
    coveredLetters[0] = letters[guess[0]];
    if (__popcnt64(coveredLetters[0]) < 16) continue;   // speedup vs. lower threshold for more exhaustive search
    if (numGuesses > 1) printf("%d \r", g0);
    guesses[0] = (const char*)word[guess[0]];

    int g1 = 0;
    while (1) {
      if (numGuesses >= 2) {
        if (g1 >= g0) break;
        guess[1] = rank[g1++];
        coveredLetters[1] = coveredLetters[0] | letters[guess[1]];
        if (__popcnt64(coveredLetters[1]) < 32) continue;
        guesses[1] = (const char*)word[guess[1]];
      }

      int g2 = 0;
      while (1) {
        if (numGuesses >= 3) {
          if (g2 >= g1) break;
          guess[2] = rank[g2++];
          coveredLetters[2] = coveredLetters[1] | letters[guess[2]];
          if (__popcnt64(coveredLetters[2]) < 48) continue;
          guesses[2] = (const char*)word[guess[2]];
        }

        double info = calcInfo(guesses);
        if (info >= bestInfo - 0.05) {
          for (int n = 0; n < numGuesses; ++n) {
            printf("%s ", word[guess[n]]);
            if (info >= bestInfo) {
              bestInfo = info;
              best[n] = guess[n];
            }
          }
          printf("%.3f %2d %2d %2d %d %d\n", info,
            (int)__popcnt64(coveredLetters[0]),
            (int)__popcnt64(letters[guess[1]]),
            (int)__popcnt64(coveredLetters[numGuesses - 1]), g0, g1);  // 11.2 bits info req'd
        }
        if (numGuesses == 2) {
          if (info >= minPairInfo)
            addPair(guess[0], guess[1], (float)info);
          break;
        }
      }
      if (numGuesses < 2) break;
    }
  }
  printf("\n");
}

int rankPairsByInfo(const void* arg1, const void* arg2) {
  return pairInfo[*(int*)arg2] > pairInfo[*(int*)arg1] ? 1 : -1;
}

int pairRank[MaxNumPairs];

void rankPairs(int list = 5) { // single word rank
  for (int i = MaxNumPairs; --i >= 0;) pairRank[i] = i;
  qsort(&pairRank, MaxNumPairs, sizeof pairRank[0], rankPairsByInfo);

  for (int i = 0; i < list; ++i)
    printf("%s %s %.3f %2d\n", word[topPair[pairRank[i]][0]], word[topPair[pairRank[i]][1]], pairInfo[pairRank[i]],
      (int)__popcnt64(letters[topPair[pairRank[i]][0]] | letters[topPair[pairRank[i]][1]]));
  printf("%.3f\n\n", minPairInfo); // can use 1% less as initial threshold
}


// vetch anglo wispy 8.313 46 1567 4801

void best3words() {
  const int numGuesses = 3;
  int guess[MaxNumGuesses]; // index
  const char* guesses[MaxNumGuesses] = { 0 };
  double bestInfo = 10.7;
  Bits coveredLetters[MaxNumGuesses];

  for (int p = 0; p < MaxNumPairs; ++p) {
    int pair = pairRank[p];
    guess[0] = topPair[pair][0];
    guess[1] = topPair[pair][1];
    if (guess[0] == 0 && guess[1] == 0) break;
    guesses[0] = (const char*)word[guess[0]];
    guesses[1] = (const char*)word[guess[1]];
    coveredLetters[1] = letters[guess[0]] | letters[guess[1]];
    printf("%d\r", p);

    for (int g2 = 0; g2 < guessCount; ++g2) { // better bcawards??  TODO
      guess[2] = rank[g2];
      guesses[2] = (const char*)word[guess[2]];
      coveredLetters[2] = coveredLetters[1] | letters[guess[2]];
      if (__popcnt64(coveredLetters[2]) < 48) continue;

      double info = calcInfo(guesses);
      if (info >= bestInfo - 0.01) {
        if (info >= bestInfo)
          bestInfo = info;
        for (int n = 0; n < numGuesses; ++n)
          printf("%s ", word[guess[n]]);
        
        printf("%.3f %2d %d %d\n", info, (int)__popcnt64(coveredLetters[numGuesses - 1]), p, g2);  // 11.2 bits info req'd
      }
    }
  }
}


int main() {
  readWords(); // printf("%d\n", targetCount);
  initSubsets();
  initLetterWeights();

  // remain(); exit(0);
  checkWords();

  rankGuesses();

  FILE* pf = fopen("pairs", "rb");
  if (!pf) {
    bestInfoGuesses(2);
    pf = fopen("pairs", "wb");
    fwrite(topPair, sizeof topPair, 1, pf);
    fwrite(pairInfo, sizeof pairInfo, 1, pf);
  } else {
    fread(topPair, sizeof topPair, 1, pf);
    fread(pairInfo, sizeof pairInfo, 1, pf);
  }
  fclose(pf);

  rankPairs();  

  best3words(); 
  exit(0);

  // bestByWeights(); exit(0);

  // matches(); exit(0);

  // bestEliminator(1); exit(0);

  exit(0);
}

/*
Feb 2022:     2,315 hidden words, 10,657 allowed guesses
NYT Mar 2022: 2,309 hidden words, 10,663 allowed guesses
removed: agora, fibre, lynch, pupal, slave, wench 

removed guesses?" darky, gooks, spics, coons, sluts, whore, bitch, pussy, ... ?re-added?

daint corse glyph 10.800 51 107 10255
daint corse plumb 10.797 50 107 10581
doilt parse bunch 10.794 50 404 9490
plant dorse chimb 10.822 50 670 10324
plant sored chimb 10.819 50 1104 10324
pored slant chimb 10.819 50 1105 10324
roped slant chimb 10.818 50 2165 10324
gored slant chimp 10.825 51 3458 9533
borde slant chimp 10.818 50 4219 9533
bored slant chimp 10.824 50 4322 9533
glint sared chomp 10.818 51 4570 9447
brant soled chimp 10.819 50 4703 9533
robed slant chimp 10.829 50 6193 9533
horst dance blimp 10.827 50 6545 9201
horde canst blimp 10.820 50 7957 9201
chant sored blimp 10.824 50 8153 9201
chant dorse blimp 10.824 50 8430 9201
blist dance morph 10.830 50 18775 6782
glint sored champ 10.825 51 18947 8205
clipt morse bandh 10.823 50 21112 7073
sabed clint morph 10.822 50 23974 6782
clasp morne dight 10.826 51 30907 8177
robed plast ginch 10.823 50 31307 8126
robed spalt ginch 10.828 50 35056 8126
clasp borne dight 10.823 50 37906 8177
spred giant clomb 10.820 50 41770 8155
shard clint pombe 10.822 50 64338 6519

*/
