/**
 * @file Grammar.hpp
 * @author Pekka Mikkola <pjmikkol@cs.helsinki.fi>
 *
 * @section LICENSE
 *
 * This file is part of bwtc.
 *
 * bwtc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bwtc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bwtc.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 *
 * Header for grammar-object used to collect information about the
 * choices done during the preprocessing.
 */

#ifndef BWTC_GRAMMAR_HPP_
#define BWTC_GRAMMAR_HPP_

#include <algorithm>
#include <vector>

#include "../globaldefs.hpp"

namespace bwtc {

/**Grammar object stores information about the replacements done
 * during the preprocessing. Preprocessors need grammar for storing the
 * special symbols and updating the right-hand sides of the rules if
 * needed (this may happen if some variable of grammar is promoted to
 * special symbol).
 */
class Grammar {
 public:
  Grammar();

  inline bool isSpecial(byte symbol) { return m_isSpecialSymbol[symbol]; }

  inline size_t numOfRules() const { return m_rules.size(); }
  
  /**Adding replacement for pair.*/
  void addRule(byte variable, byte first, byte second);

  /**Adding replacement for longer string.*/
  void addRule(byte variable, byte *begin, size_t length);

  inline uint32 specialSymbolPairsLeft() const {
    uint32 s = m_specialSymbols.size();
    return s*s - m_specialPairReplacements.size();
  }

  void writeGrammar(byte* dst) const;
  uint32 writeRightSides(byte* dst) const;
  uint32 writeLengthsOfRules(byte* dst) const;
  uint32 writeVariables(byte* dst) const;
  uint32 writeLargeVariableFlags(byte* dst) const;
  uint32 writeNumberOfRules(byte* dst) const;

 private:
  /**Rule stores single replacement. They are chosen by preprocessing
   * algoritms.*/
  class Rule {
   public:
    Rule(uint16 variable, uint32 begin, uint32 end, bool largeVariable=false)
        : m_begin(begin), m_end(end), m_variable(variable),
          m_largeVariable(largeVariable) { }

    inline uint32 begin() const {return m_begin;}
    inline uint16 end() const {return m_end;}
    inline uint16 variable() const {return m_variable;}
    inline bool isLarge() const { return m_largeVariable; }
    inline void changeVariable(uint16 nVariable) { m_variable = nVariable; }
    inline size_t length() const { return m_end - m_begin; }

   private:
    /**Starting index in Grammar::m_rightHandSides of replaced string.*/
    uint32 m_begin;
    /**One past last index in Grammar::m_rightHandSides replaced string.*/
    uint32 m_end;
    uint16 m_variable;
    /**Variable is large if it's formed of two special symbols.
     * Otherwise the actual value of m_variable represents the actual
     * replacement character.*/
    bool m_largeVariable;
  };

  /**Frequencies of bytes in the right-side of rules.*/
  uint32 m_frequencies[256];
  bool m_isSpecialSymbol[256];
  bool m_isVariable[256];
  std::vector<byte> m_specialSymbols;
  /**Special symbols are numbered in order they are created.
   * If pair.first==true then pair of this index is used as a grammar
   * variable. */
  std::vector<std::pair<bool,byte> > m_specialPairReplacements;

  std::vector<Rule> m_rules;
  /**Right-hand sides of the rules.*/
  std::vector<byte> m_rightHandSides;

  uint32 m_specialSymbolsAsVariables;

};

} //namespace bwtc

#endif
