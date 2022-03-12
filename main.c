#include <stdio.h>
#include "9cc.h"

// 入力プログラム
char *user_input;

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
    return 1;
  }

  // トークナイズしてパースする
  user_input = argv[1];
  tokenize();
  program();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  for(int i = 0; func_info[i]; i++) {
    cur_func_info = func_info[i];
    gen_func();
  }
  return 0;
}