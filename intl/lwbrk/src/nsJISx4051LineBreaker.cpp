/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#include "nsJISx4051LineBreaker.h"

#include "jisx4051class.h"
#include "nsComplexBreaker.h"
#include "nsTArray.h"

/* 

   Simplification of Pair Table in JIS X 4051

   1. The Origion Table - in 4.1.3

   In JIS x 4051. The pair table is defined as below

   Class of
   Leading    Class of Trailing Char Class
   Char        

              1  2  3  4  5  6  7  8  9 10 11 12 13 13 14 14 15 16 17 18 19 20
                                                 *  #  *  #
        1     X  X  X  X  X  X  X  X  X  X  X  X  X  X  X  X  X  X  X  X  X  E
        2        X  X  X  X  X                                               X
        3        X  X  X  X  X                                               X
        4        X  X  X  X  X                                               X
        5        X  X  X  X  X                                               X
        6        X  X  X  X  X                                               X
        7        X  X  X  X  X  X                                            X
        8        X  X  X  X  X                                X              E
        9        X  X  X  X  X                                               X
       10        X  X  X  X  X                                               X
       11        X  X  X  X  X                                               X
       12        X  X  X  X  X                                               X
       13        X  X  X  X  X                    X                          X
       14        X  X  X  X  X                          X                    X
       15        X  X  X  X  X        X                       X        X     X
       16        X  X  X  X  X                                   X     X     X
       17        X  X  X  X  X                                               E
       18        X  X  X  X  X                                X  X     X     X
       19     X  E  E  E  E  E  X  X  X  X  X  X  X  X  X  X  X  X  E  X  E  E
       20        X  X  X  X  X                                               E

   * Same Char
   # Other Char

   X Cannot Break

   The classes mean:
      1: Open parenthesis
      2: Close parenthesis
      3: Prohibit a line break before
      4: Punctuation for sentence end (except Full stop, e.g., "!" and "?")
      5: Middle dot (e.g., U+30FB KATAKANA MIDDLE DOT)
      6: Full stop
      7: Non-breakable between same characters
      8: Prefix (e.g., "$", "NO.")
      9: Postfix (e.g., "%")
     10: Ideographic space
     11: Hiragana
     12: Japanese characters (except class 11)
     13: Subscript
     14: Ruby
     15: Numeric
     16: Alphabet
     17: Space for Western language
     18: Western characters (except class 17)
     19: Split line note (Warichu) begin quote
     20: Split line note (Warichu) end quote

   2. Simplified by remove the class which we do not care

   However, since we do not care about class 13(Subscript), 14(Ruby),
   16 (Aphabet), 19(split line note begin quote), and 20(split line note end
   quote) we can simplify this par table into the following

   Class of
   Leading    Class of Trailing Char Class
   Char

              1  2  3  4  5  6  7  8  9 10 11 12 15 17 18

        1     X  X  X  X  X  X  X  X  X  X  X  X  X  X  X
        2        X  X  X  X  X                           
        3        X  X  X  X  X                           
        4        X  X  X  X  X                           
        5        X  X  X  X  X                           
        6        X  X  X  X  X                           
        7        X  X  X  X  X  X                        
        8        X  X  X  X  X                    X      
        9        X  X  X  X  X                           
       10        X  X  X  X  X                           
       11        X  X  X  X  X                           
       12        X  X  X  X  X                           
       15        X  X  X  X  X        X           X     X
       17        X  X  X  X  X                           
       18        X  X  X  X  X                    X     X

   3. Simplified by merged classes

   After the 2 simplification, the pair table have some duplication
   a. class 2, 3, 4, 5, 6,  are the same- we can merged them
   b. class 10, 11, 12, 17  are the same- we can merged them


   Class of
   Leading    Class of Trailing Char Class
   Char

              1 [a] 7  8  9 [b]15 18

        1     X  X  X  X  X  X  X  X
      [a]        X                  
        7        X  X               
        8        X              X   
        9        X                  
      [b]        X                  
       15        X        X     X  X
       18        X              X  X


   4. We add COMPLEX characters and make it breakable w/ all ther class
      except after class 1 and before class [a]

   Class of
   Leading    Class of Trailing Char Class
   Char

              1 [a] 7  8  9 [b]15 18 COMPLEX

        1     X  X  X  X  X  X  X  X  X
      [a]        X                     
        7        X  X                  
        8        X              X      
        9        X                     
      [b]        X                     
       15        X        X     X  X   
       18        X              X  X   
  COMPLEX        X                    T

     T : need special handling


   5. However, we need two special class for some punctuations/parentheses,
      theirs breaking rules like character class (18), see bug 389056.
      And also we need character like punctuation that is same behavior with 18,
      but the characters are not letters of all languages. (e.g., '_')
      [c]. Based on open parenthesis class (1), but it is not breakable after
           character class (18) or numeric class (15).
      [d]. Based on close parenthesis (or punctuation) class (2), but it is not
           breakable before character class (18) or numeric class (15).

   Class of
   Leading    Class of Trailing Char Class
   Char

              1 [a] 7  8  9 [b]15 18 COMPLEX [c] [d]

        1     X  X  X  X  X  X  X  X  X       X    X
      [a]        X                            X    X
        7        X  X                               
        8        X              X                   
        9        X                                  
      [b]        X                                 X
       15        X        X     X  X          X    X
       18        X              X  X          X    X
  COMPLEX        X                    T             
      [c]     X  X  X  X  X  X  X  X  X       X    X
      [d]        X              X  X               X


   6. And Unicode has "NON-BREAK" characters. The lines should be broken around
      them. But in JIS X 4051, such class is not, therefore, we create [e].

   Class of
   Leading    Class of Trailing Char Class
   Char

              1 [a] 7  8  9 [b]15 18 COMPLEX [c] [d] [e]

        1     X  X  X  X  X  X  X  X  X       X    X   X
      [a]        X                                 X   X
        7        X  X                                  X
        8        X              X                      X
        9        X                                     X
      [b]        X                                 X   X
       15        X        X     X  X          X    X   X
       18        X              X  X          X    X   X
  COMPLEX        X                    T                X
      [c]     X  X  X  X  X  X  X  X  X       X    X   X
      [d]        X              X  X               X   X
      [e]     X  X  X  X  X  X  X  X  X       X    X   X


   7. Now we use one bit to encode weather it is breakable, and use 2 bytes
      for one row, then the bit table will look like:

                 18    <-   1

       1  0000 1111 1111 1111  = 0x0FFF
      [a] 0000 1100 0000 0010  = 0x0C02
       7  0000 1000 0000 0110  = 0x0806
       8  0000 1000 0100 0010  = 0x0842
       9  0000 1000 0000 0010  = 0x0802
      [b] 0000 1100 0000 0010  = 0x0C02
      15  0000 1110 1101 0010  = 0x0ED2
      18  0000 1110 1100 0010  = 0x0EC2
 COMPLEX  0000 1001 0000 0010  = 0x0902
      [c] 0000 1111 1111 1111  = 0x0FFF
      [d] 0000 1100 1100 0010  = 0x0CC2
      [e] 0000 1111 1111 1111  = 0x0FFF
*/

#define MAX_CLASSES 12

static const uint16_t gPair[MAX_CLASSES] = {
  0x0FFF,
  0x0C02,
  0x0806,
  0x0842,
  0x0802,
  0x0C02,
  0x0ED2,
  0x0EC2,
  0x0902,
  0x0FFF,
  0x0CC2,
  0x0FFF
};


/*

   8. And if the character is not enough far from word start, word end and
      another break point, we should not break in non-CJK languages.
      I.e., Don't break around 15, 18, [c] and [d], but don't change
      that if they are related to [b].

   Class of
   Leading    Class of Trailing Char Class
   Char

              1 [a] 7  8  9 [b]15 18 COMPLEX [c] [d] [e]

        1     X  X  X  X  X  X  X  X  X       X    X   X
      [a]        X              X  X          X    X   X
        7        X  X           X  X          X    X   X
        8        X              X  X          X    X   X
        9        X              X  X          X    X   X
      [b]        X                                 X   X
       15     X  X  X  X  X     X  X  X       X    X   X
       18     X  X  X  X  X     X  X  X       X    X   X
  COMPLEX        X              X  X  T       X    X   X
      [c]     X  X  X  X  X  X  X  X  X       X    X   X
      [d]     X  X  X  X  X     X  X  X       X    X   X
      [e]     X  X  X  X  X  X  X  X  X       X    X   X

                 18    <-   1

       1  0000 1111 1111 1111  = 0x0FFF
      [a] 0000 1110 1100 0010  = 0x0EC2
       7  0000 1110 1100 0110  = 0x0EC6
       8  0000 1110 1100 0010  = 0x0EC2
       9  0000 1110 1100 0010  = 0x0EC2
      [b] 0000 1100 0000 0010  = 0x0C02
      15  0000 1111 1101 1111  = 0x0FDF
      18  0000 1111 1101 1111  = 0x0FDF
 COMPLEX  0000 1111 1100 0010  = 0x0FC2
      [c] 0000 1111 1111 1111  = 0x0FFF
      [d] 0000 1111 1101 1111  = 0x0FDF
      [e] 0000 1111 1111 1111  = 0x0FFF
*/

static const uint16_t gPairConservative[MAX_CLASSES] = {
  0x0FFF,
  0x0EC2,
  0x0EC6,
  0x0EC2,
  0x0EC2,
  0x0C02,
  0x0FDF,
  0x0FDF,
  0x0FC2,
  0x0FFF,
  0x0FDF,
  0x0FFF
};


/*

   9. Now we map the class to number

      0: 1 
      1: [a]- 2, 3, 4, 5, 6
      2: 7
      3: 8
      4: 9
      5: [b]- 10, 11, 12, 17
      6: 15
      7: 18
      8: COMPLEX
      9: [c]
      A: [d]
      B: [e]

    and they mean:
      0: Open parenthesis
      1: Punctuation that prohibits break before
      2: Non-breakable between same classes
      3: Prefix
      4: Postfix
      5: Breakable character (Spaces and Most Japanese characters)
      6: Numeric
      7: Characters
      8: Need special handling characters (E.g., Thai)
      9: Open parentheses like Character (See bug 389056)
      A: Close parenthese (or punctuations) like Character (See bug 389056)
      B: Non breakable (See bug 390920)

*/

#define CLASS_NONE                             INT8_MAX

#define CLASS_OPEN                             0x00
#define CLASS_CLOSE                            0x01
#define CLASS_NON_BREAKABLE_BETWEEN_SAME_CLASS 0x02
#define CLASS_PREFIX                           0x03
#define CLASS_POSTFFIX                         0x04
#define CLASS_BREAKABLE                        0x05
#define CLASS_NUMERIC                          0x06
#define CLASS_CHARACTER                        0x07
#define CLASS_COMPLEX                          0x08
#define CLASS_OPEN_LIKE_CHARACTER              0x09
#define CLASS_CLOSE_LIKE_CHARACTER             0x0A
#define CLASS_NON_BREAKABLE                    0x0B

#define U_NULL      char16_t(0x0000)
#define U_SLASH     char16_t('/')
#define U_SPACE     char16_t(' ')
#define U_HYPHEN    char16_t('-')
#define U_EQUAL     char16_t('=')
#define U_PERCENT   char16_t('%')
#define U_AMPERSAND char16_t('&')
#define U_SEMICOLON char16_t(';')
#define U_BACKSLASH char16_t('\\')
#define U_OPEN_SINGLE_QUOTE char16_t(0x2018)
#define U_OPEN_DOUBLE_QUOTE char16_t(0x201C)
#define U_OPEN_GUILLEMET    char16_t(0x00AB)

#define NEED_CONTEXTUAL_ANALYSIS(c) (IS_HYPHEN(c) || \
                                     (c) == U_SLASH || \
                                     (c) == U_PERCENT || \
                                     (c) == U_AMPERSAND || \
                                     (c) == U_SEMICOLON || \
                                     (c) == U_BACKSLASH || \
                                     (c) == U_OPEN_SINGLE_QUOTE || \
                                     (c) == U_OPEN_DOUBLE_QUOTE || \
                                     (c) == U_OPEN_GUILLEMET)

#define IS_ASCII_DIGIT(u) (0x0030 <= (u) && (u) <= 0x0039)

static inline int
GETCLASSFROMTABLE(const uint32_t* t, uint16_t l)
{
  return ((((t)[(l>>3)]) >> ((l & 0x0007)<<2)) & 0x000f);
}

static inline int
IS_HALFWIDTH_IN_JISx4051_CLASS3(char16_t u)
{
  return ((0xff66 <= (u)) && ((u) <= 0xff70));
}

static inline int
IS_CJK_CHAR(char16_t u)
{
  return ((0x1100 <= (u) && (u) <= 0x11ff) ||
          (0x2e80 <= (u) && (u) <= 0xd7ff) ||
          (0xf900 <= (u) && (u) <= 0xfaff) ||
          (0xff00 <= (u) && (u) <= 0xffef) );
}

static inline bool
IS_NONBREAKABLE_SPACE(char16_t u)
{
  return u == 0x00A0 || u == 0x2007; // NO-BREAK SPACE, FIGURE SPACE
}

static inline bool
IS_HYPHEN(char16_t u)
{
  return (u == U_HYPHEN ||
          u == 0x058A || // ARMENIAN HYPHEN
          u == 0x2010 || // HYPHEN
          u == 0x2012 || // FIGURE DASH
          u == 0x2013);  // EN DASH
}

static int8_t
GetClass(char16_t u)
{
   uint16_t h = u & 0xFF00;
   uint16_t l = u & 0x00ff;
   int8_t c;

   // Handle 3 range table first
   if (0x0000 == h) {
     c = GETCLASSFROMTABLE(gLBClass00, l);
   } else if (0x1700 == h) {
     c = GETCLASSFROMTABLE(gLBClass17, l);
   } else if (NS_NeedsPlatformNativeHandling(u)) {
     c = CLASS_COMPLEX;
   } else if (0x0E00 == h) {
     c = GETCLASSFROMTABLE(gLBClass0E, l);
   } else if (0x2000 == h) {
     c = GETCLASSFROMTABLE(gLBClass20, l);
   } else if (0x2100 == h) {
     c = GETCLASSFROMTABLE(gLBClass21, l);
   } else if (0x3000 == h) {
     c = GETCLASSFROMTABLE(gLBClass30, l);
   } else if (((0x3200 <= u) && (u <= 0xA4CF)) || // CJK and Yi
              ((0xAC00 <= h) && (h <= 0xD7FF)) || // Hangul
              ((0xf900 <= h) && (h <= 0xfaff))) {
     c = CLASS_BREAKABLE; // CJK character, Han, and Han Compatibility
   } else if (0xff00 == h) {
     if (l < 0x0060) { // Fullwidth ASCII variant
       c = GETCLASSFROMTABLE(gLBClass00, (l+0x20));
     } else if (l < 0x00a0) {
       switch (l) {
         case 0x61: c = GetClass(0x3002); break;
         case 0x62: c = GetClass(0x300c); break;
         case 0x63: c = GetClass(0x300d); break;
         case 0x64: c = GetClass(0x3001); break;
         case 0x65: c = GetClass(0x30fb); break;
         case 0x9e: c = GetClass(0x309b); break;
         case 0x9f: c = GetClass(0x309c); break;
         default:
           if (IS_HALFWIDTH_IN_JISx4051_CLASS3(u))
              c = CLASS_CLOSE; // jis x4051 class 3
           else
              c = CLASS_BREAKABLE; // jis x4051 class 11
           break;
       }
     // Halfwidth Katakana variants
     } else if (l < 0x00e0) {
       c = CLASS_CHARACTER; // Halfwidth Hangul variants
     } else if (l < 0x00f0) {
       static char16_t NarrowFFEx[16] = {
         0x00A2, 0x00A3, 0x00AC, 0x00AF, 0x00A6, 0x00A5, 0x20A9, 0x0000,
         0x2502, 0x2190, 0x2191, 0x2192, 0x2193, 0x25A0, 0x25CB, 0x0000
       };
       c = GetClass(NarrowFFEx[l - 0x00e0]);
     } else {
       c = CLASS_CHARACTER;
     }
   } else if (0x3100 == h) { 
     if (l <= 0xbf) { // Hangul Compatibility Jamo, Bopomofo, Kanbun
                      // XXX: This is per UAX #14, but UAX #14 may change
                      // the line breaking rules about Kanbun and Bopomofo.
       c = CLASS_BREAKABLE;
     } else if (l >= 0xf0) { // Katakana small letters for Ainu
       c = CLASS_CLOSE;
     } else { // unassigned
       c = CLASS_CHARACTER;
     }
   } else if (0x0300 == h) {
     if (0x4F == l || (0x5C <= l && l <= 0x62))
       c = CLASS_NON_BREAKABLE;
     else
       c = CLASS_CHARACTER;
   } else if (0x0500 == h) {
     // ARMENIAN HYPHEN (for "Breaking Hyphens" of UAX#14)
     if (l == 0x8A)
       c = GETCLASSFROMTABLE(gLBClass00, uint16_t(U_HYPHEN));
     else
       c = CLASS_CHARACTER;
   } else if (0x0F00 == h) {
     if (0x08 == l || 0x0C == l || 0x12 == l)
       c = CLASS_NON_BREAKABLE;
     else
       c = CLASS_CHARACTER;
   } else if (0x1800 == h) {
     if (0x0E == l)
       c = CLASS_NON_BREAKABLE;
     else
       c = CLASS_CHARACTER;
   } else if (0x1600 == h) {
     if (0x80 == l) { // U+1680 OGHAM SPACE MARK
       c = CLASS_BREAKABLE;
     } else {
       c = CLASS_CHARACTER;
     }
   } else if (u == 0xfeff) {
     c = CLASS_NON_BREAKABLE;
   } else {
     c = CLASS_CHARACTER; // others
   }
   return c;
}

static bool
GetPair(int8_t c1, int8_t c2)
{
  NS_ASSERTION(c1 < MAX_CLASSES ,"illegal classes 1");
  NS_ASSERTION(c2 < MAX_CLASSES ,"illegal classes 2");

  return (0 == ((gPair[c1] >> c2) & 0x0001));
}

static bool
GetPairConservative(int8_t c1, int8_t c2)
{
  NS_ASSERTION(c1 < MAX_CLASSES ,"illegal classes 1");
  NS_ASSERTION(c2 < MAX_CLASSES ,"illegal classes 2");

  return (0 == ((gPairConservative[c1] >> c2) & 0x0001));
}

nsJISx4051LineBreaker::nsJISx4051LineBreaker()
{
}

nsJISx4051LineBreaker::~nsJISx4051LineBreaker()
{
}

NS_IMPL_ISUPPORTS1(nsJISx4051LineBreaker, nsILineBreaker)

class ContextState {
public:
  ContextState(const char16_t* aText, uint32_t aLength) {
    mUniText = aText;
    mText = nullptr;
    mLength = aLength;
    Init();
  }

  ContextState(const uint8_t* aText, uint32_t aLength) {
    mUniText = nullptr;
    mText = aText;
    mLength = aLength;
    Init();
  }

  uint32_t Length() { return mLength; }
  uint32_t Index() { return mIndex; }

  char16_t GetCharAt(uint32_t aIndex) {
    NS_ASSERTION(aIndex < mLength, "Out of range!");
    return mUniText ? mUniText[aIndex] : char16_t(mText[aIndex]);
  }

  void AdvanceIndex() {
    ++mIndex;
  }

  void NotifyBreakBefore() { mLastBreakIndex = mIndex; }

// A word of western language should not be broken. But even if the word has
// only ASCII characters, non-natural context words should be broken, e.g.,
// URL and file path. For protecting the natural words, we should use
// conservative breaking rules at following conditions:
//   1. at near the start of word
//   2. at near the end of word
//   3. at near the latest broken point
// CONSERVATIVE_BREAK_RANGE define the 'near' in characters.
#define CONSERVATIVE_BREAK_RANGE 6

  bool UseConservativeBreaking(uint32_t aOffset = 0) {
    if (mHasCJKChar)
      return false;
    uint32_t index = mIndex + aOffset;
    bool result = (index < CONSERVATIVE_BREAK_RANGE ||
                     mLength - index < CONSERVATIVE_BREAK_RANGE ||
                     index - mLastBreakIndex < CONSERVATIVE_BREAK_RANGE);
    if (result || !mHasNonbreakableSpace)
      return result;

    // This text has no-breakable space, we need to check whether the index
    // is near it.

    // Note that index is always larger than CONSERVATIVE_BREAK_RANGE here.
    for (uint32_t i = index; index - CONSERVATIVE_BREAK_RANGE < i; --i) {
      if (IS_NONBREAKABLE_SPACE(GetCharAt(i - 1)))
        return true;
    }
    // Note that index is always less than mLength - CONSERVATIVE_BREAK_RANGE.
    for (uint32_t i = index + 1; i < index + CONSERVATIVE_BREAK_RANGE; ++i) {
      if (IS_NONBREAKABLE_SPACE(GetCharAt(i)))
        return true;
    }
    return false;
  }

  bool HasPreviousEqualsSign() const {
    return mHasPreviousEqualsSign;
  }
  void NotifySeenEqualsSign() {
    mHasPreviousEqualsSign = true;
  }

  bool HasPreviousSlash() const {
    return mHasPreviousSlash;
  }
  void NotifySeenSlash() {
    mHasPreviousSlash = true;
  }

  bool HasPreviousBackslash() const {
    return mHasPreviousBackslash;
  }
  void NotifySeenBackslash() {
    mHasPreviousBackslash = true;
  }

  char16_t GetPreviousNonHyphenCharacter() const {
    return mPreviousNonHyphenCharacter;
  }
  void NotifyNonHyphenCharacter(char16_t ch) {
    mPreviousNonHyphenCharacter = ch;
  }

private:
  void Init() {
    mIndex = 0;
    mLastBreakIndex = 0;
    mPreviousNonHyphenCharacter = U_NULL;
    mHasCJKChar = 0;
    mHasNonbreakableSpace = 0;
    mHasPreviousEqualsSign = false;
    mHasPreviousSlash = false;
    mHasPreviousBackslash = false;

    for (uint32_t i = 0; i < mLength; ++i) {
      char16_t u = GetCharAt(i);
      if (!mHasNonbreakableSpace && IS_NONBREAKABLE_SPACE(u))
        mHasNonbreakableSpace = 1;
      else if (mUniText && !mHasCJKChar && IS_CJK_CHAR(u))
        mHasCJKChar = 1;
    }
  }

  const char16_t* mUniText;
  const uint8_t* mText;

  uint32_t mIndex;
  uint32_t mLength;         // length of text
  uint32_t mLastBreakIndex;
  char16_t mPreviousNonHyphenCharacter; // The last character we have seen
                                         // which is not U_HYPHEN
  bool mHasCJKChar; // if the text has CJK character, this is true.
  bool mHasNonbreakableSpace; // if the text has no-breakable space,
                                     // this is true.
  bool mHasPreviousEqualsSign; // True if we have seen a U_EQUAL
  bool mHasPreviousSlash;      // True if we have seen a U_SLASH
  bool mHasPreviousBackslash;  // True if we have seen a U_BACKSLASH
};

static int8_t
ContextualAnalysis(char16_t prev, char16_t cur, char16_t next,
                   ContextState &aState)
{
  // Don't return CLASS_OPEN/CLASS_CLOSE if aState.UseJISX4051 is FALSE.

  if (IS_HYPHEN(cur)) {
    // If next character is hyphen, we don't need to break between them.
    if (IS_HYPHEN(next))
      return CLASS_CHARACTER;
    // If prev and next characters are numeric, it may be in Math context.
    // So, we should not break here.
    bool prevIsNum = IS_ASCII_DIGIT(prev);
    bool nextIsNum = IS_ASCII_DIGIT(next);
    if (prevIsNum && nextIsNum)
      return CLASS_NUMERIC;
    // If one side is numeric and the other is a character, or if both sides are
    // characters, the hyphen should be breakable.
    if (!aState.UseConservativeBreaking(1)) {
      char16_t prevOfHyphen = aState.GetPreviousNonHyphenCharacter();
      if (prevOfHyphen && next) {
        bool prevIsChar = !NEED_CONTEXTUAL_ANALYSIS(prevOfHyphen) &&
                            GetClass(prevOfHyphen) == CLASS_CHARACTER;
        bool nextIsChar = !NEED_CONTEXTUAL_ANALYSIS(next) &&
                            GetClass(next) == CLASS_CHARACTER;
        if ((prevIsNum || prevIsChar) && (nextIsNum || nextIsChar))
          return CLASS_CLOSE;
      }
    }
  } else {
    aState.NotifyNonHyphenCharacter(cur);
    if (cur == U_SLASH || cur == U_BACKSLASH) {
      // If this is immediately after same char, we should not break here.
      if (prev == cur)
        return CLASS_CHARACTER;
      // If this text has two or more (BACK)SLASHs, this may be file path or URL.
      // Make sure to compute shouldReturn before we notify on this slash.
      bool shouldReturn = !aState.UseConservativeBreaking() &&
        (cur == U_SLASH ?
         aState.HasPreviousSlash() : aState.HasPreviousBackslash());

      if (cur == U_SLASH) {
        aState.NotifySeenSlash();
      } else {
        aState.NotifySeenBackslash();
      }

      if (shouldReturn)
        return CLASS_OPEN;
    } else if (cur == U_PERCENT) {
      // If this is a part of the param of URL, we should break before.
      if (!aState.UseConservativeBreaking()) {
        if (aState.Index() >= 3 &&
            aState.GetCharAt(aState.Index() - 3) == U_PERCENT)
          return CLASS_OPEN;
        if (aState.Index() + 3 < aState.Length() &&
            aState.GetCharAt(aState.Index() + 3) == U_PERCENT)
          return CLASS_OPEN;
      }
    } else if (cur == U_AMPERSAND || cur == U_SEMICOLON) {
      // If this may be a separator of params of URL, we should break after.
      if (!aState.UseConservativeBreaking(1) &&
          aState.HasPreviousEqualsSign())
        return CLASS_CLOSE;
    } else if (cur == U_OPEN_SINGLE_QUOTE ||
               cur == U_OPEN_DOUBLE_QUOTE ||
               cur == U_OPEN_GUILLEMET) {
      // for CJK usage, we treat these as openers to allow a break before them,
      // but otherwise treat them as normal characters because quote mark usage
      // in various Western languages varies too much; see bug #450088 discussion.
      if (!aState.UseConservativeBreaking() && IS_CJK_CHAR(next))
        return CLASS_OPEN;
    } else {
      NS_ERROR("Forgot to handle the current character!");
    }
  }
  return GetClass(cur);
}


int32_t
nsJISx4051LineBreaker::WordMove(const char16_t* aText, uint32_t aLen,
                                uint32_t aPos, int8_t aDirection)
{
  bool    textNeedsJISx4051 = false;
  int32_t begin, end;

  for (begin = aPos; begin > 0 && !NS_IsSpace(aText[begin - 1]); --begin) {
    if (IS_CJK_CHAR(aText[begin]) || NS_NeedsPlatformNativeHandling(aText[begin])) {
      textNeedsJISx4051 = true;
    }
  }
  for (end = aPos + 1; end < int32_t(aLen) && !NS_IsSpace(aText[end]); ++end) {
    if (IS_CJK_CHAR(aText[end]) || NS_NeedsPlatformNativeHandling(aText[end])) {
      textNeedsJISx4051 = true;
    }
  }

  int32_t ret;
  nsAutoTArray<uint8_t, 2000> breakState;
  if (!textNeedsJISx4051 || !breakState.AppendElements(end - begin)) {
    // No complex text character, do not try to do complex line break.
    // (This is required for serializers. See Bug #344816.)
    // Also fall back to this when out of memory.
    if (aDirection < 0) {
      ret = (begin == int32_t(aPos)) ? begin - 1 : begin;
    } else {
      ret = end;
    }
  } else {
    GetJISx4051Breaks(aText + begin, end - begin, nsILineBreaker::kWordBreak_Normal,
                      breakState.Elements());

    ret = aPos;
    do {
      ret += aDirection;
    } while (begin < ret && ret < end && !breakState[ret - begin]);
  }

  return ret;
}

int32_t
nsJISx4051LineBreaker::Next(const char16_t* aText, uint32_t aLen,
                            uint32_t aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen > aPos, "Bad position passed to nsJISx4051LineBreaker::Next");

  int32_t nextPos = WordMove(aText, aLen, aPos, 1);
  return nextPos < int32_t(aLen) ? nextPos : NS_LINEBREAKER_NEED_MORE_TEXT;
}

int32_t
nsJISx4051LineBreaker::Prev(const char16_t* aText, uint32_t aLen,
                            uint32_t aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen >= aPos && aPos > 0,
               "Bad position passed to nsJISx4051LineBreaker::Prev");

  int32_t prevPos = WordMove(aText, aLen, aPos, -1);
  return prevPos > 0 ? prevPos : NS_LINEBREAKER_NEED_MORE_TEXT;
}

void
nsJISx4051LineBreaker::GetJISx4051Breaks(const char16_t* aChars, uint32_t aLength,
                                         uint8_t aWordBreak,
                                         uint8_t* aBreakBefore)
{
  uint32_t cur;
  int8_t lastClass = CLASS_NONE;
  ContextState state(aChars, aLength);

  for (cur = 0; cur < aLength; ++cur, state.AdvanceIndex()) {
    char16_t ch = aChars[cur];
    int8_t cl;

    if (NEED_CONTEXTUAL_ANALYSIS(ch)) {
      cl = ContextualAnalysis(cur > 0 ? aChars[cur - 1] : U_NULL,
                              ch,
                              cur + 1 < aLength ? aChars[cur + 1] : U_NULL,
                              state);
    } else {
      if (ch == U_EQUAL)
        state.NotifySeenEqualsSign();
      state.NotifyNonHyphenCharacter(ch);
      cl = GetClass(ch);
    }

    bool allowBreak = false;
    if (cur > 0) {
      NS_ASSERTION(CLASS_COMPLEX != lastClass || CLASS_COMPLEX != cl,
                   "Loop should have prevented adjacent complex chars here");
      if (aWordBreak == nsILineBreaker::kWordBreak_Normal) {
        allowBreak = (state.UseConservativeBreaking()) ?
          GetPairConservative(lastClass, cl) : GetPair(lastClass, cl);
      } else if (aWordBreak == nsILineBreaker::kWordBreak_BreakAll) {
        allowBreak = true;
      }
    }
    aBreakBefore[cur] = allowBreak;
    if (allowBreak)
      state.NotifyBreakBefore();
    lastClass = cl;
    if (CLASS_COMPLEX == cl) {
      uint32_t end = cur + 1;

      while (end < aLength && CLASS_COMPLEX == GetClass(aChars[end])) {
        ++end;
      }

      NS_GetComplexLineBreaks(aChars + cur, end - cur, aBreakBefore + cur);

      // We have to consider word-break value again for complex characters
      if (aWordBreak != nsILineBreaker::kWordBreak_Normal) {
        // Respect word-break property 
        for (uint32_t i = cur; i < end; i++)
          aBreakBefore[i] = (aWordBreak == nsILineBreaker::kWordBreak_BreakAll);
      }

      // restore breakability at chunk begin, which was always set to false
      // by the complex line breaker
      aBreakBefore[cur] = allowBreak;

      cur = end - 1;
    }
  }
}

void
nsJISx4051LineBreaker::GetJISx4051Breaks(const uint8_t* aChars, uint32_t aLength,
                                         uint8_t aWordBreak,
                                         uint8_t* aBreakBefore)
{
  uint32_t cur;
  int8_t lastClass = CLASS_NONE;
  ContextState state(aChars, aLength);

  for (cur = 0; cur < aLength; ++cur, state.AdvanceIndex()) {
    char16_t ch = aChars[cur];
    int8_t cl;

    if (NEED_CONTEXTUAL_ANALYSIS(ch)) {
      cl = ContextualAnalysis(cur > 0 ? aChars[cur - 1] : U_NULL,
                              ch,
                              cur + 1 < aLength ? aChars[cur + 1] : U_NULL,
                              state);
    } else {
      if (ch == U_EQUAL)
        state.NotifySeenEqualsSign();
      state.NotifyNonHyphenCharacter(ch);
      cl = GetClass(ch);
    }

    bool allowBreak = false;
    if (cur > 0) {
      if (aWordBreak == nsILineBreaker::kWordBreak_Normal) {
        allowBreak = (state.UseConservativeBreaking()) ?
          GetPairConservative(lastClass, cl) : GetPair(lastClass, cl);
      } else if (aWordBreak == nsILineBreaker::kWordBreak_BreakAll) {
        allowBreak = true;
      }
    }
    aBreakBefore[cur] = allowBreak;
    if (allowBreak)
      state.NotifyBreakBefore();
    lastClass = cl;
  }
}
