#include <stdio.h>
#include "9cc.h"

int lbl_num = 1; //ラベルの番号
char *arg_reg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen_lval(Node *node) {
  if (node->kind == ND_LVAR) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
  } else if(node->kind == ND_DEREF) {
    gen_lval(node->child[0]);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
  } else {
    error("代入の左辺が適切な形式ではありません");
  }
}

void gen(Node *node) {
  int i;
  Node *arg;
  if(node == NULL) return ;
  switch (node->kind) {
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_LVAR:
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ADDR:
    gen_lval(node->child[0]);
    return;
  case ND_DEREF:
    gen(node->child[0]);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ASSIGN:
    gen_lval(node->child[0]);
    gen(node->child[1]);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  case ND_IF:
    if(node->child[2]) {
      gen(node->child[0]);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("je  .Lelse%d\n",lbl_num);
      gen(node->child[1]);
      printf("jmp .Lend%d\n",lbl_num+1);
      printf(".Lelse%d:\n",lbl_num);
      gen(node->child[2]);
      printf(".Lend%d:\n",lbl_num+1);
      lbl_num += 2;
    } else {
      gen(node->child[0]);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("je  .Lend%d\n",lbl_num);
      gen(node->child[1]);
      printf(".Lend%d:\n",lbl_num);
      lbl_num++;
    }
    return;
  case ND_WHILE:
    printf(".Lbegin%d:\n",lbl_num);  
    gen(node->child[0]);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("je  .Lend%d\n",lbl_num+1);
    gen(node->child[1]);
    printf("jmp .Lbegin%d\n",lbl_num);
    printf(".Lend%d:\n",lbl_num+1); 
    lbl_num += 2;
    return;
  case ND_FOR:
    gen(node->child[0]);
    printf(".Lbegin%d:\n",lbl_num);  
    gen(node->child[1]);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("je  .Lend%d\n",lbl_num+1);
    gen(node->child[3]);
    gen(node->child[2]);
    printf("jmp .Lbegin%d\n",lbl_num);
    printf(".Lend%d:\n",lbl_num+1); 
    lbl_num += 2;
    return;
  case ND_BLOCK:
    while (node = node->next) {
      gen(node);
    }
    return;
  case ND_RETURN:
    gen(node->child[0]);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;   
  case ND_CALL:
    i=0;
    arg = node->args;
    while(arg) {
      gen(arg);
      printf("  pop %s\n", arg_reg[i++]);
      arg = arg->args;
    }
    printf("  mov rax, rsp\n");
    printf("  cqo\n");
    printf("  mov r8, 16\n");
    printf("  idiv r8\n");
    printf("  sub rsp, rdx\n");
    printf("  call ");
    char *func_name = node->func_name;
    for(i=0;i<node->func_name_len;i++) { 
      printf("%c",*func_name);
      func_name++;
    }
    printf("\n");
    printf("  push rax\n");
    return;
  }

  gen(node->child[0]);
  gen(node->child[1]);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;  
  }

  printf("  push rax\n");
}

// 関数のコード生成
void gen_func() {
  int i;
  // 関数名
  printf(".globl ");
  for(i = 0; i < cur_func_info->name_len; i++)
    printf("%c", cur_func_info->name[i]);
  printf("\n");
  for(i = 0; i < cur_func_info->name_len; i++)
    printf("%c", cur_func_info->name[i]);
  printf(":\n");

  // プロローグ
  // 変数26個分の領域を確保する
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", cur_func_info->locals->offset);

  // 引数をレジスタからスタックにコピーする
  for(i = 0; i < cur_func_info->arg_num; i++) {
    printf("  mov rax, -8\n");
    printf("  mov [rbp + rax * %d], %s\n", i+1, arg_reg[i]);
  }
  // 先頭の式から順にコード生成
  for (i = 0; cur_func_info->stmt[i]; i++) 
    gen(cur_func_info->stmt[i]);
  
}