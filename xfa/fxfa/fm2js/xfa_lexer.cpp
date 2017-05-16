// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/fm2js/xfa_lexer.h"

#include "core/fxcrt/fx_extension.h"
#include "third_party/base/ptr_util.h"

namespace {

bool IsValid(const wchar_t* p) {
  return *p == 0 || (*p >= 0x09 && *p <= 0x0D) ||
         (*p >= 0x20 && *p <= 0xd7FF) || (*p >= 0xE000 && *p <= 0xFFFD);
}

const XFA_FMKeyword keyWords[] = {
    {TOKand, 0x00000026, L"&"},
    {TOKlparen, 0x00000028, L"("},
    {TOKrparen, 0x00000029, L")"},
    {TOKmul, 0x0000002a, L"*"},
    {TOKplus, 0x0000002b, L"+"},
    {TOKcomma, 0x0000002c, L","},
    {TOKminus, 0x0000002d, L"-"},
    {TOKdot, 0x0000002e, L"."},
    {TOKdiv, 0x0000002f, L"/"},
    {TOKlt, 0x0000003c, L"<"},
    {TOKassign, 0x0000003d, L"="},
    {TOKgt, 0x0000003e, L">"},
    {TOKlbracket, 0x0000005b, L"["},
    {TOKrbracket, 0x0000005d, L"]"},
    {TOKor, 0x0000007c, L"|"},
    {TOKdotscream, 0x0000ec11, L".#"},
    {TOKdotstar, 0x0000ec18, L".*"},
    {TOKdotdot, 0x0000ec1c, L".."},
    {TOKle, 0x000133f9, L"<="},
    {TOKne, 0x000133fa, L"<>"},
    {TOKeq, 0x0001391a, L"=="},
    {TOKge, 0x00013e3b, L">="},
    {TOKdo, 0x00020153, L"do"},
    {TOKkseq, 0x00020676, L"eq"},
    {TOKksge, 0x000210ac, L"ge"},
    {TOKksgt, 0x000210bb, L"gt"},
    {TOKif, 0x00021aef, L"if"},
    {TOKin, 0x00021af7, L"in"},
    {TOKksle, 0x00022a51, L"le"},
    {TOKkslt, 0x00022a60, L"lt"},
    {TOKksne, 0x00023493, L"ne"},
    {TOKksor, 0x000239c1, L"or"},
    {TOKnull, 0x052931bb, L"null"},
    {TOKbreak, 0x05518c25, L"break"},
    {TOKksand, 0x09f9db33, L"and"},
    {TOKend, 0x0a631437, L"end"},
    {TOKeof, 0x0a63195a, L"eof"},
    {TOKfor, 0x0a7d67a7, L"for"},
    {TOKnan, 0x0b4f91dd, L"nan"},
    {TOKksnot, 0x0b4fd9b1, L"not"},
    {TOKvar, 0x0c2203e9, L"var"},
    {TOKthen, 0x2d5738cf, L"then"},
    {TOKelse, 0x45f65ee9, L"else"},
    {TOKexit, 0x4731d6ba, L"exit"},
    {TOKdownto, 0x4caadc3b, L"downto"},
    {TOKreturn, 0x4db8bd60, L"return"},
    {TOKinfinity, 0x5c0a010a, L"infinity"},
    {TOKendwhile, 0x5c64bff0, L"endwhile"},
    {TOKforeach, 0x67e31f38, L"foreach"},
    {TOKendfunc, 0x68f984a3, L"endfunc"},
    {TOKelseif, 0x78253218, L"elseif"},
    {TOKwhile, 0x84229259, L"while"},
    {TOKendfor, 0x8ab49d7e, L"endfor"},
    {TOKthrow, 0x8db05c94, L"throw"},
    {TOKstep, 0xa7a7887c, L"step"},
    {TOKupto, 0xb5155328, L"upto"},
    {TOKcontinue, 0xc0340685, L"continue"},
    {TOKfunc, 0xcdce60ec, L"func"},
    {TOKendif, 0xe0e8fee6, L"endif"},
};

const XFA_FM_TOKEN KEYWORD_START = TOKdo;
const XFA_FM_TOKEN KEYWORD_END = TOKendif;

}  // namespace

const wchar_t* XFA_FM_KeywordToString(XFA_FM_TOKEN op) {
  if (op < KEYWORD_START || op > KEYWORD_END)
    return L"";
  return keyWords[op].m_keyword;
}

CXFA_FMToken::CXFA_FMToken() : m_type(TOKreserver), m_uLinenum(1) {}

CXFA_FMToken::CXFA_FMToken(uint32_t uLineNum)
    : m_type(TOKreserver), m_uLinenum(uLineNum) {}

CXFA_FMLexer::CXFA_FMLexer(const CFX_WideStringC& wsFormCalc,
                           CXFA_FMErrorInfo* pErrorInfo)
    : m_ptr(wsFormCalc.c_str()), m_uCurrentLine(1), m_pErrorInfo(pErrorInfo) {}

CXFA_FMLexer::~CXFA_FMLexer() {}

CXFA_FMToken* CXFA_FMLexer::NextToken() {
  m_pToken = Scan();
  return m_pToken.get();
}

std::unique_ptr<CXFA_FMToken> CXFA_FMLexer::Scan() {
  uint16_t ch = 0;
  auto p = pdfium::MakeUnique<CXFA_FMToken>(m_uCurrentLine);
  if (!IsValid(m_ptr)) {
    ch = *m_ptr;
    Error(kFMErrUnsupportedChar, ch);
    return p;
  }

  while (1) {
    ch = *m_ptr;
    if (!IsValid(m_ptr)) {
      Error(kFMErrUnsupportedChar, ch);
      return p;
    }

    switch (ch) {
      case 0:
        p->m_type = TOKeof;
        return p;
      case 0x0A:
        ++m_uCurrentLine;
        p->m_uLinenum = m_uCurrentLine;
        ++m_ptr;
        break;
      case 0x0D:
        ++m_ptr;
        break;
      case ';': {
        m_ptr = Comment(m_ptr);
        break;
      }
      case '"': {
        p->m_type = TOKstring;
        m_ptr = String(p.get(), m_ptr);
        return p;
      }
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        p->m_type = TOKnumber;
        m_ptr = Number(p.get(), m_ptr);
        return p;
      }
      case '=':
        ++m_ptr;
        if (IsValid(m_ptr)) {
          ch = *m_ptr;
          if (ch == '=') {
            p->m_type = TOKeq;
            ++m_ptr;
          } else {
            p->m_type = TOKassign;
          }
        } else {
          ch = *m_ptr;
          Error(kFMErrUnsupportedChar, ch);
        }
        return p;
      case '<':
        ++m_ptr;
        if (IsValid(m_ptr)) {
          ch = *m_ptr;
          if (ch == '=') {
            p->m_type = TOKle;
            ++m_ptr;
          } else if (ch == '>') {
            p->m_type = TOKne;
            ++m_ptr;
          } else {
            p->m_type = TOKlt;
          }
        } else {
          ch = *m_ptr;
          Error(kFMErrUnsupportedChar, ch);
        }
        return p;
      case '>':
        ++m_ptr;
        if (IsValid(m_ptr)) {
          ch = *m_ptr;
          if (ch == '=') {
            p->m_type = TOKge;
            ++m_ptr;
          } else {
            p->m_type = TOKgt;
          }
        } else {
          ch = *m_ptr;
          Error(kFMErrUnsupportedChar, ch);
        }
        return p;
      case ',':
        p->m_type = TOKcomma;
        ++m_ptr;
        return p;
      case '(':
        p->m_type = TOKlparen;
        ++m_ptr;
        return p;
      case ')':
        p->m_type = TOKrparen;
        ++m_ptr;
        return p;
      case '[':
        p->m_type = TOKlbracket;
        ++m_ptr;
        return p;
      case ']':
        p->m_type = TOKrbracket;
        ++m_ptr;
        return p;
      case '&':
        ++m_ptr;
        p->m_type = TOKand;
        return p;
      case '|':
        ++m_ptr;
        p->m_type = TOKor;
        return p;
      case '+':
        ++m_ptr;
        p->m_type = TOKplus;
        return p;
      case '-':
        ++m_ptr;
        p->m_type = TOKminus;
        return p;
      case '*':
        ++m_ptr;
        p->m_type = TOKmul;
        return p;
      case '/': {
        ++m_ptr;
        if (!IsValid(m_ptr)) {
          ch = *m_ptr;
          Error(kFMErrUnsupportedChar, ch);
          return p;
        }
        ch = *m_ptr;
        if (ch != '/') {
          p->m_type = TOKdiv;
          return p;
        }
        m_ptr = Comment(m_ptr);
        break;
      }
      case '.':
        ++m_ptr;
        if (IsValid(m_ptr)) {
          ch = *m_ptr;
          if (ch == '.') {
            p->m_type = TOKdotdot;
            ++m_ptr;
          } else if (ch == '*') {
            p->m_type = TOKdotstar;
            ++m_ptr;
          } else if (ch == '#') {
            p->m_type = TOKdotscream;
            ++m_ptr;
          } else if (ch <= '9' && ch >= '0') {
            p->m_type = TOKnumber;
            --m_ptr;
            m_ptr = Number(p.get(), m_ptr);
          } else {
            p->m_type = TOKdot;
          }
        } else {
          ch = *m_ptr;
          Error(kFMErrUnsupportedChar, ch);
        }
        return p;
      case 0x09:
      case 0x0B:
      case 0x0C:
      case 0x20:
        ++m_ptr;
        break;
      default: {
        m_ptr = Identifiers(p.get(), m_ptr);
        return p;
      }
    }
  }
}

const wchar_t* CXFA_FMLexer::Number(CXFA_FMToken* t, const wchar_t* p) {
  // This will set pEnd to the character after the end of the number.
  wchar_t* pEnd = nullptr;
  if (p)
    wcstod(const_cast<wchar_t*>(p), &pEnd);
  if (pEnd && FXSYS_iswalpha(*pEnd)) {
    Error(kFMErrBadSuffixNumber);
    return pEnd;
  }

  t->m_wstring = CFX_WideStringC(p, (pEnd - p));
  return pEnd;
}

const wchar_t* CXFA_FMLexer::String(CXFA_FMToken* t, const wchar_t* p) {
  const wchar_t* pStart = p;

  ++p;
  uint16_t ch = *p;
  while (ch) {
    if (!IsValid(p)) {
      ch = *p;
      t->m_wstring = CFX_WideStringC(pStart, (p - pStart));
      Error(kFMErrUnsupportedChar, ch);
      return p;
    }
    if (ch != '"') {
      ++p;
      ch = *p;
      continue;
    }

    ++p;
    if (!IsValid(p)) {
      ch = *p;
      t->m_wstring = CFX_WideStringC(pStart, (p - pStart));
      Error(kFMErrUnsupportedChar, ch);
      return p;
    }
    ch = *p;
    if (ch != '"')
      break;

    ++p;
    ch = *p;
  }
  t->m_wstring = CFX_WideStringC(pStart, (p - pStart));
  return p;
}

const wchar_t* CXFA_FMLexer::Identifiers(CXFA_FMToken* t, const wchar_t* p) {
  const wchar_t* pStart = p;
  uint16_t ch = *p;
  ++p;
  if (!IsValid(p)) {
    t->m_wstring = CFX_WideStringC(pStart, (p - pStart));
    Error(kFMErrUnsupportedChar, ch);
    return p;
  }

  ch = *p;
  while (ch) {
    if (!IsValid(p)) {
      t->m_wstring = CFX_WideStringC(pStart, (p - pStart));
      Error(kFMErrUnsupportedChar, ch);
      return p;
    }

    ch = *p;
    if (ch == 0 || ch == 0x0A || ch == 0x0D || ch == 0x09 || ch == 0x0B ||
        ch == 0x0C || ch == 0x20 || ch == '.' || ch == ';' || ch == '"' ||
        ch == '=' || ch == '<' || ch == '>' || ch == ',' || ch == '(' ||
        ch == ')' || ch == ']' || ch == '[' || ch == '&' || ch == '|' ||
        ch == '+' || ch == '-' || ch == '*' || ch == '/') {
      break;
    }
    ++p;
  }
  t->m_wstring = CFX_WideStringC(pStart, (p - pStart));
  t->m_type = IsKeyword(t->m_wstring);
  return p;
}

const wchar_t* CXFA_FMLexer::Comment(const wchar_t* p) {
  ++p;
  unsigned ch = *p;
  while (ch) {
    if (ch == 0x0D) {
      ++p;
      return p;
    }
    if (ch == 0x0A) {
      ++m_uCurrentLine;
      ++p;
      return p;
    }
    ++p;
    ch = *p;
  }
  return p;
}

XFA_FM_TOKEN CXFA_FMLexer::IsKeyword(const CFX_WideStringC& str) {
  uint32_t uHash = FX_HashCode_GetW(str, true);
  int32_t iStart = KEYWORD_START;
  int32_t iEnd = KEYWORD_END;
  do {
    int32_t iMid = (iStart + iEnd) / 2;
    XFA_FMKeyword keyword = keyWords[iMid];
    if (uHash == keyword.m_uHash)
      return keyword.m_type;
    if (uHash < keyword.m_uHash)
      iEnd = iMid - 1;
    else
      iStart = iMid + 1;
  } while (iStart <= iEnd);
  return TOKidentifier;
}

void CXFA_FMLexer::Error(const wchar_t* msg, ...) {
  m_pErrorInfo->linenum = m_uCurrentLine;
  va_list ap;
  va_start(ap, msg);
  m_pErrorInfo->message.FormatV(msg, ap);
  va_end(ap);
}

bool CXFA_FMLexer::HasError() const {
  return !m_pErrorInfo->message.IsEmpty();
}
