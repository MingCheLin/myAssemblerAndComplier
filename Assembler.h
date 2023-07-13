#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "Cpu0.h"

typedef struct{
    Array* codes;
    HashTable *symTable, *opTable;
} Assembler;

typedef struct{
    char *line;
    int  address;
    int opCode;
    int size;
    char *label, *op, type;
    Array *tokens;
    int  argStart;
    char *objCode; 
    char *arg[3];
    int argNum;
    int  r[3];
    int  rCount;
    int  cx;
} AsmCode;  

void assembler(char *asmFile, char *objFile);

Assembler* AsmNew();
void AsmFree(Assembler *a);

void AsmPass1(Assembler *a, char *text);
void AsmPass2(Assembler *a);
void AsmSaveObjFile(Assembler *a, char *objFile);
void AsmTranslateCode(Assembler *a, AsmCode *code);
AsmCode *AsmCodeNew(char *line);
void AsmCodeFree(AsmCode *code);
int AsmCodePrintln(AsmCode *code);
int AsmCodeSize(AsmCode *code);
void AsmCodeParseD(AsmCode* code, Assembler *a);

#endif