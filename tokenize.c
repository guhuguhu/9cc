#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "9cc.h"

// 現在着目しているトークン
Token *token;

int is_alnum(char c) {
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') ||
         (c == '_');
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

// 入力文字列pをトークナイズしてそれを返す
void tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;
  char str[20];
  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
        cur = new_token(TK_RETURN, cur, p);
        p += 6;
        cur->len = 6;
        continue;
    }

    if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
        cur = new_token(TK_IF, cur, p);
        p += 2;
        cur->len = 2;
        continue;
    }

    if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
        cur = new_token(TK_ELSE, cur, p);
        p += 4;
        cur->len = 4;
        continue;
    }

    if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
        cur = new_token(TK_WHILE, cur, p);
        p += 5;
        cur->len = 5;
        continue;
    }

    if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
        cur = new_token(TK_FOR, cur, p);
        p += 3;
        cur->len = 3;
        continue;
    }

    if (!strncmp(p, ">=", 2) || !strncmp(p, "<=", 2) || 
        !strncmp(p, "==", 2) || !strncmp(p, "!=", 2)) {
        cur = new_token(TK_RESERVED, cur, p);
        p += 2;
        cur->len = 2;
        continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
        *p == '(' || *p == ')' || *p == '>' || *p == '<' ||
        *p == '=' || *p == ';' || *p == '{' || *p == '}' ||
        *p == ',' || *p == '*' || *p == '&') {
      cur = new_token(TK_RESERVED, cur, p++);
      cur->len = 1;
      continue;
    }

    char *first = p;
    while ('a' <= *p && *p <= 'z')
        p++;
    if (p > first) {
      cur = new_token(TK_IDENT, cur, first);
      cur->len = p-first;
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }


    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p);
  token = head.next;
}