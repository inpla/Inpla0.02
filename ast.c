//----------------------------------------
// for AST
//----------------------------------------
//http://www.hpcs.cs.tsukuba.ac.jp/~msato/lecture-note/comp-lecture/tiny-c-note2.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

#define MAX_SYMBOLS 256
typedef struct {
  char* sym[MAX_SYMBOLS];
  int val[MAX_SYMBOLS];
  int ptr;
} SymbolTable;

SymbolTable SymTable, ConstTable;


void SymTable_init(SymbolTable *table) {
  int i;
  table->ptr = 0;
  for (i=0; i<MAX_SYMBOLS; i++) {
    table->sym[i] = NULL;
  }
}

char *lookupSymbol(SymbolTable *table, char *name) {
  int i;
  char *result;

  for (i=0; i< table->ptr; i++) {
    if (strcmp(table->sym[i], name) == 0) {
      return table->sym[i];
    }
  }

  // inpla.l �� strdup ����Ă���̂ŁA
  // �����ł� stardup �����ɁA�P�� �|�C���^���i�[���邾���B
  table->sym[table->ptr]=name; 
  result = table->sym[table->ptr];
  table->ptr++;
  if (table->ptr > MAX_SYMBOLS) {
    puts("ERROR: The SymbolTable in AST library is too small.");
    exit(-1);
  }

  return result;
}

int lookupEntry(SymbolTable *table, char *name) {
  int i;

  for (i=0; i< table->ptr; i++) {
    if (strcmp(table->sym[i], name) == 0) {
      return i;
    }
  }
  return -1;
}

void recordVal(SymbolTable *table, char *name, int val) {
  int i;

  for (i=0; i< table->ptr; i++) {
    if (strcmp(table->sym[i], name) == 0) {
      table->val[i] = val;
      return;
    }
  }
  table->sym[table->ptr]=strdup(name);
  table->val[table->ptr]=val;
  table->ptr++;
  if (table->ptr > MAX_SYMBOLS) {
    puts("ERROR: The SymbolTable in AST library is too small.");
    exit(-1);
  }

}


static Ast *AstHeap;
static int NextPtr_AstHeap;
#define MAX_AST_HEAP 10000

void ast_heapInit(void) {
  
  NextPtr_AstHeap = 0;
  AstHeap = malloc(sizeof(Ast)*MAX_AST_HEAP);
  if (AstHeap == NULL) {
    printf("Malloc error [AstHeap]\n");
    exit(-1);
  }

  SymTable_init(&SymTable);
  SymTable_init(&ConstTable);
}

void ast_heapReInit(void) {
  NextPtr_AstHeap = 0;
}  

static Ast *ast_myalloc(void) {
  Ast *ptr;
  
  if (NextPtr_AstHeap < MAX_AST_HEAP) {
    ptr=&AstHeap[NextPtr_AstHeap];
    NextPtr_AstHeap++;
  } else {
    printf("[Error] All memory for AST was run out.\n");
    exit(-1);
  }

  return ptr;
}


Ast *ast_makeSymbol(char *name) {
  Ast *ptr;
  ptr = ast_myalloc();
  ptr->id = AST_SYM;
  ptr->sym = lookupSymbol(&SymTable, name);
  return ptr;
}

Ast *ast_makeInt(int num) {
  Ast *ptr;
  ptr = ast_myalloc();
  ptr->id = AST_INT;
  ptr->intval = num;
  return ptr;
}

void ast_recordConst(char *name, int val) {
  recordVal(&ConstTable, name, val);
}


Ast *ast_makeAST(AST_ID id, Ast *left, Ast *right) {
  Ast *ptr;
  if ((id == AST_AGENT) && (right == NULL)) {
    int entry = lookupEntry(&ConstTable, left->sym);
    if (entry != -1) {
      ptr = ast_myalloc();
      ptr->id = AST_INT;
      ptr->intval = ConstTable.val[entry];
      return ptr;
    }
  }
  ptr = ast_myalloc();
  ptr->id = id;
  ptr->right = right;
  ptr->left = left;
  return ptr;
}

int count_elem(Ast *p) {
  int count=0;
  while (p!=NULL) {
    p = ast_getTail(p);
    count++;
  }
  return count;
}

Ast *ast_makeTuple(Ast *tuple) {
    Ast *ptr;
    ptr = ast_myalloc();
    ptr->id = AST_TUPLE;
    ptr->right = tuple;
    ptr->intval = count_elem(tuple);
    return ptr;
}

Ast *ast_paramToCons(Ast *ast) { 
  // ast �� List(a, List(b,NULL)) �Ƃ��� astlist �������Ă���Ƃ��āA
  // ����� AST_CONS(nonsym, a, AST_CONS(nonsym, b, AST_NIL())) �ƕϊ�����

  Ast *at = ast;
  while (ast != NULL) {
    at->id = AST_CONS;
    if (at->right == NULL) {
      Ast *ptr;
      ptr = ast_myalloc();
      ptr->id = AST_NIL;
      at->right = ptr;
      break;
    }
    at=at->right;
  }
  return ast;
}



Ast *ast_addLast(Ast *l, Ast *p)
{
    Ast *q;

    if(l == NULL) return ast_makeAST(AST_LIST,p,NULL);
    q = l;
    while(q->right != NULL) q = q->right;
    q->right = ast_makeAST(AST_LIST,p,NULL);
    return l;
}

Ast *ast_getNth(Ast *p,int nth)
{
    if(p->id != AST_LIST){
	fprintf(stderr,"bad access to list\n");
	exit(1);
    }
    if(nth > 0) return(ast_getNth(p->right,nth-1));
    else return p->left;
}

Ast *ast_getTail(Ast *p)
{
    if(p->id != AST_LIST){
	fprintf(stderr,"bad access to list\n");
	exit(1);
    }
    else return p->right;
}

void puts_ast(Ast *p) {
  static char *string_AstID[] = {
    // basic
    "SYM", "NAME", "INTNAME", "AGENT", "AP", "RULE", "LIST", 

    // annotation
    "(*L)", "(*R)",

    // extension
    "TUPLE", 
    "INT", "LD", "ADD", "SUB", "MUL", "DIV", "MOD", 
    "LT", "LE",  "EQ", "NE", "UNM", 
    "CONS", "NIL", "OPCONS",
    "RAND", "SRAND", 

    // default
    "UNDEFINED",
  };

  if (p==NULL) {printf("NULL"); return;}

  switch(p->id) {
  case AST_INT:
    printf("int %d", p->intval);
    break;
  case AST_SYM:
    printf("%s", p->sym);
    break;
  case AST_TUPLE:
    printf("TUPLE%d(", p->intval);
    puts_ast(p->right);
    printf(")");
    break;
  case AST_NIL:
    printf("NIL");
    break;
  case AST_ANNOTATION_L:
  case AST_ANNOTATION_R:
    printf("%s", string_AstID[p->id]);
    puts_ast(p->left);
    break;
  default:
    printf("%s(", string_AstID[p->id]);
    puts_ast(p->left);
    printf(",");
    puts_ast(p->right);
    printf(")");

  }
}
