#include <stdio.h>
#include "9cc.h"

int lbl_num = 1; //ラベルの番号

void gen_lval(Node *node) {
  if (node->kind != ND_LVAR)
    error("代入の左辺値が変数ではありません");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen(Node *node) {
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