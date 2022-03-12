// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数トークン
  TK_RETURN,   // return
  TK_IF,       // if
  TK_ELSE,     // else
  TK_WHILE,    // while
  TK_FOR,      // for
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ
};

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_LT,  // <
  ND_LE,  // <=
  ND_EQ,  // ==
  ND_NE,  // != 
  ND_ASSIGN, // =
  ND_LVAR,   // ローカル変数
  ND_IF,     // if
  ND_WHILE,  // while
  ND_FOR,    // for
  ND_RETURN, // return
  ND_BLOCK, // {}
  ND_CALL, // 関数呼び出し
  ND_NUM, // 整数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
  NodeKind kind; // ノードの型
  Node *child[4]; // 子ノード
  Node *next;    // kindがND_BLOCKの場合、ブロック内の文。kindがND_CALLの場合、引数。
  int val;       // kindがND_NUMの場合のみ使う
  int offset;    // kindがND_LVARの場合のみ使う
  char *func_name; // kindがND_CALLの場合のみ使う。
  int func_name_len; // kindがND_CALLの場合のみ使う。
  Node *args;     // kindがND_CALLの場合のみ使う。
};

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
  LVar *next; // 次の変数かNULL
  char *name; // 変数の名前
  int len;    // 名前の長さ
  int offset; // RBPからのオフセット
};

extern Token *token;

extern char *user_input;

extern Node *code[100];

extern LVar *locals;

void tokenize();
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void program();
void gen(Node *node);

