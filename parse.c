#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

// 関数の情報
Func *func_info[100];

// 現在着目している関数
Func *cur_func_info;

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " "); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
  if (token->kind == TK_IDENT || token->kind == TK_NUM || 
      token->kind == TK_EOF ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// 次のトークンが識別子のときには、トークンを1つ読み進めて
// そのトークンを返す。それ以外の場合にはNULLを返す。
Token *consume_ident() {
    if(token->kind == TK_IDENT) {
        Token *t = token;
        token = token->next;
        return t;
    }
    else 
        return NULL;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
  if (token->kind == TK_IDENT || token->kind == TK_NUM || 
      token->kind == TK_EOF || 
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "\"%s\"ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok) {
  for (LVar *var = cur_func_info->locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

Func *find_func(char *name, int len) {
  int i = 0;
  for (int i = 0; func_info[i]; i++) {
    if (func_info[i]->name_len == len && !memcmp(func_info[i]->name, name, len))
      return func_info[i];     
  }
  return NULL;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->child[0] = lhs;
  node->child[1] = rhs;
  switch (kind) {
  case ND_ADD:
    if (lhs->type->ty == INT && rhs->type->ty == INT)
      node->type = lhs->type;
    else if ((lhs->type->ty == PTR || lhs->type->ty == ARRAY) && 
              rhs->type->ty == INT) 
      node->type = lhs->type;
    else if (lhs->type->ty == INT && 
             (rhs->type->ty == PTR || rhs->type->ty == ARRAY)) 
      node->type = rhs->type;
    else 
      error_at(token->str, "オペランドの型が妥当ではありません");
    return node;
  case ND_SUB:
    if (lhs->type->ty == INT && rhs->type->ty == INT)
      node->type = lhs->type;
    else if ((lhs->type->ty == PTR || lhs->type->ty == ARRAY) && 
              rhs->type->ty == INT) 
      node->type = lhs->type;
    else  
      error_at(token->str, "オペランドの型が妥当ではありません");
    return node;
  case ND_ASSIGN:
    if (lhs->type->ty == rhs->type->ty)
      node->type = rhs->type;
    else if (lhs->type->ty == PTR && rhs->type->ty == ARRAY)
      node->type = rhs->type;
    else 
      error_at(token->str, "オペランドの型が妥当ではありません");
    return node;
  default:
    if (lhs->type->ty == INT && rhs->type->ty == INT)
      node->type = lhs->type;
    else 
      error_at(token->str, "オペランドの型が妥当ではありません");
    return node;
  }
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  node->type = calloc(1, sizeof(Type));
  node->type->ty = INT;
  return node;
}

Node *expr();
Node *add();

Node *primary() {
  // "(" expr ")"
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();

  // (ident "(" ")") | (ident "(" ident ("," ident)* ")")
  if (tok && consume("(")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_CALL;
    node->func_name = tok->str;
    node->func_name_len = tok->len;
    Func *func = find_func(tok->str, tok->len);
    if (!func) 
      error_at(tok->str, "未宣言の関数です");
    node->type = func->ret_type;
    Node *n = node;
    if (consume(")")) 
      return node;
    while (true) {
      n->args = expr();
      if (consume(")"))
        return node;
      expect(",");
      n = n->args; 
    }
  }

  // ident ("[" (num | ident) "]")*
  if (tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    LVar *lvar = find_lvar(tok);
    if (lvar) {
      node->offset = lvar->offset;
      node->type = lvar->type;
    } else {
      error_at(tok->str, "宣言されていない変数です");
    }

    while (consume("[")) {
      Node *n = add();
      expect("]");
      Node *add_node = new_node(ND_ADD, node, n);
      Node *der_node = calloc(1, sizeof(Node));
      der_node->kind = ND_DEREF;
      der_node->type = add_node->type->ptr_to;
      der_node->child[0] = add_node;
      node = der_node;
    }
    return node;
  }

  // num
  return new_node_num(expect_number());
}

// ("+" primary) | ("-" primary) | ("*" unary) | ("&" unary)
Node *unary() {
  if (consume("+"))
    return primary();
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), primary());
  if (consume("*")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_DEREF;
    node->child[0] = unary();
    node->type = node->child[0]->type->ptr_to;
    return node;
  }
  if (consume("&")) {
    Node *ch = unary();
    if (ch->type->ty == ARRAY) 
      return ch;
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_ADDR;
    node->child[0] = ch;
    node->type = calloc(1, sizeof(Type));
    node->type->ty = PTR;
    node->type->ptr_to = node->child[0]->type;
    return node;
  }

  if (consume("sizeof")) {
    Node *node = unary();
    int all_size = 1;
    Type *t;
    if (node->type->ty == INT) 
      return new_node_num(4);
    else if (node->type->ty == PTR)
      return new_node_num(8);
    else if (node->type->ty == ARRAY) {
      return new_node_num(node->type->array_size);
    }
  }
  return primary();
}

// unary (("*" | "/") unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// mul (("+" | "-") mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// add (("<" | "<=" | ">" | ">=") add)*
Node *rel() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_LT, node, add());
    else if (consume("<="))
      node = new_node(ND_LE, node, add());
    else if (consume(">"))
      node = new_node(ND_LT, add(), node);
    else if (consume(">="))
      node = new_node(ND_LE, add(), node);    
    else
      return node;
  }
}

// rel (("==" | "!=") rel)*
Node *equal() {
  Node *node = rel();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, rel());
    else if (consume("!="))
      node = new_node(ND_NE, node, rel());
    else
      return node;
  }
}

// equal ("=" assign)?
Node *assign() {
  Node *node = equal();
  if (consume("="))
    node = new_node(ND_ASSIGN, node, assign());
  return node;
}

// assign
Node *expr() {
  return assign();
}

Node *stmt() {
  Node *node;

// "{" stmt* "}"
  if (consume("{")) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
    Node *n = node; 
    while (!consume("}")) {
      n->next = stmt();
      n = n->next;
    }
    return node;
  }

// "if" "(" expr ")" stmt ("else" stmt)?
  if (consume("if")) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    expect("(");
    node->child[0] = expr();
    expect(")");
    node->child[1] = stmt();
    if (consume("else")) {
      node->child[2] = stmt();
    } else {
      node->child[2] = NULL;
    }
    return node;
  }

// "while" "(" expr ")" stmt
  if(consume("while")) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect("(");
    node->child[0] = expr();
    expect(")");
    node->child[1] = stmt();
    return node;
  }

// "for" "(" expr? ";" expr? ";" expr? ")" stmt
  if(consume("for")) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    expect("(");
    if(consume(";")) {
      node->child[0] = NULL;
    } else {
      node->child[0] = expr();
      expect(";");
    }
    if(consume(";")) {
      node->child[1] = NULL;
    } else {
      node->child[1] = expr();
      expect(";");
    }
    if(consume(")")) {
      node->child[2] = NULL;
    } else {
      node->child[2] = expr();
      expect(")");
    }
    node->child[3] = stmt();
    return node;
  }

// "return" expr ;
  if (consume("return")) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->child[0] = expr();
    expect(";");
    return node;
  }

// "int" "*"* ident ("[" num "]")* ";"
  if (consume("int")) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_DEC;
    Type *type = calloc(1, sizeof(Type));
    type->ty = INT;
    while(consume("*")) {
      Type *t = calloc(1, sizeof(Type));
      t->ty = PTR;
      t->ptr_to = type;
      type = t;
    }
    
    Token *tok = consume_ident();
    if (!tok) 
      error_at(token->str, "識別子ではありません");
    int i = 0;
    int array_size[10];
    if (consume("[")) {
      array_size[i++] = expect_number();
      expect("]");
      Type *t = calloc(1, sizeof(Type));
      t->ty = ARRAY;
      t->ptr_to = type;
      type = t;
    }
    Type *t1 = type;
    while(consume("[")) {
      array_size[i++] = expect_number();
      expect("]");
      Type *t = calloc(1, sizeof(Type));
      t->ty = ARRAY;
      t->ptr_to = t1->ptr_to;
      t1->ptr_to = t;
      t1 = t;
    }
    i--;
    if (i >= 0) {
      switch (t1->ptr_to->ty) {
      case INT:
        array_size[i] *= 4;
        break;
      case PTR:
        array_size[i] *= 8;
        break;
      }
    }
    for (i--;i>=0;i--) 
      array_size[i] *= array_size[i+1];
    i = 0;
    for (t1 = type; t1->ty == ARRAY; t1 = t1->ptr_to) 
      t1->array_size = array_size[i++];

    int capacity;
    switch (type->ty) {
    case INT:
    case PTR:
      capacity = 8;
      break;
    case ARRAY:
      capacity = type->array_size + (8 - type->array_size % 8) % 8;
      break;
    default:
      error_at(token->str, "対応していない型です");
    }

    node->child[0] = calloc(1, sizeof(Node));
    node->child[0]->kind = ND_LVAR;
  
    LVar *lvar = find_lvar(tok);
    if (lvar) 
      error_at(tok->str, "変数を多重定義しています");
    lvar = calloc(1, sizeof(LVar));
    lvar->next = cur_func_info->locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->offset = cur_func_info->locals->offset + capacity;
    lvar->type = type;
    node->child[0]->offset = lvar->offset;
    cur_func_info->locals = lvar;

    expect(";");
    return node;
  }

  // expr ;
  node =expr();
  expect(";");
  return node;
}

// ("int" ident "(" ")") | ("int" ident "(" "int" ident ("," "int" ident)* ")") "{" stmt* "}"
void func() {
  cur_func_info->locals = calloc(1, sizeof(LVar));
  cur_func_info->locals->offset = 0;
  int i = 0;
  expect("int");
  Type *type = calloc(1, sizeof(Type));
  type->ty = INT;
  while(consume("*")) {
    Type *t = calloc(1, sizeof(Type));
    t->ty = PTR;
    t->ptr_to = type;
    type = t;
  }
  cur_func_info->ret_type = type;
  Token *tok = consume_ident();
  if (!tok) error_at(token->str, "関数定義ではありません\n");
  cur_func_info->name = tok->str;
  cur_func_info->name_len = tok->len;
  cur_func_info->arg_num = 0;
  expect("(");
  if(!consume(")")) {
    expect("int");
    Type *type = calloc(1, sizeof(Type));
    type->ty = INT;
    while(consume("*")) {
      Type *t = calloc(1, sizeof(Type));
      t->ty = PTR;
      t->ptr_to = type;
      type = t;
    }
    Token *lo_tok = consume_ident();
    if(!lo_tok) error_at(token->str, "識別子ではありません\n");
    LVar *lo_var = calloc(1, sizeof(LVar));
    lo_var->next = cur_func_info->locals;
    lo_var->name = lo_tok->str;
    lo_var->len = lo_tok->len;
    lo_var->offset = cur_func_info->locals->offset + 8;
    lo_var->type = type;
    cur_func_info->locals = lo_var;
    cur_func_info->arg_num++;
    while (!consume(")")) {
      expect(",");
      expect("int");
      Type *type = calloc(1, sizeof(Type));
      type->ty = INT;
      while(consume("*")) {
        Type *t = calloc(1, sizeof(Type));
        t->ty = PTR;
        t->ptr_to = type;
        type = t;
      }   
      lo_tok = consume_ident();
      if(!lo_tok) error_at(token->str, "識別子ではありません\n");
      lo_var = calloc(1, sizeof(LVar));
      lo_var->next = cur_func_info->locals;
      lo_var->name = lo_tok->str;
      lo_var->len = lo_tok->len;
      lo_var->offset = cur_func_info->locals->offset + 8;
      lo_var->type = type;
      cur_func_info->locals = lo_var;
      cur_func_info->arg_num++;
    }
  }
  expect("{");
  while (!consume("}"))
    cur_func_info->stmt[i++] = stmt();
  cur_func_info->stmt[i] = NULL;
}

// func* 
void program() {
  int i = 0;
  while (!at_eof()) {
    func_info[i] = calloc(1, sizeof(Func));
    cur_func_info = func_info[i];
    func();
    i++;
  }
  func_info[i] = NULL;
}


