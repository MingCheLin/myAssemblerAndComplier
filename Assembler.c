#include "Assembler.h"

void assembler(char* asmFile, char* objFile){
	printf("%s -> %s\n", asmFile, objFile);
	char *text = fileToStr(asmFile);  
	Assembler *a = AsmNew();
	AsmPass1(a, text);  
	
	HashTableEach(a->symTable, (FuncPtr1) AsmCodePrintln);
	AsmPass2(a);
	AsmSaveObjFile(a, objFile);
	AsmFree(a);
	freeMemory(text);
}

Assembler* AsmNew() {
	Assembler *a=ObjNew(Assembler, 1);		// 創建assembler物件
	a->codes = ArrayNew(4);				// 創建動態陣列用以紀錄組與特徵用來轉換成機器語言,建立的陣列大小並不影響
	a->symTable = HashTableNew(127);		// 創建空的標記表，用hash table的方式記錄
	a->opTable = OpTableNew();				// 創立對照的運算碼表
	return a;
}

void AsmPass1(Assembler *a, char *text){
	int address; //紀錄目前address到哪
	Array* lines = split(text, "\r\n", REMOVE_SPLITER);	// 把讀進來的以行分隔
	for (int i=0; i<lines->count; i++){			// 對每一行分別記錄指令、建立符號表、紀錄記憶體位址
		//第一部分:字串前處理把tab或;刪掉變成\0與空格
		char* line = lines->item[i];
		for (int j=0; j<strlen(line); j++){
			if (line[j] == ';')	// 假如遇到';'代表後面的都不用組譯
				line[j] = '\0';	
			else if (line[j] == '\t')	// 遇到'\t'時把'\t'消除
				line[j] = ' ';
		}
		AsmCode *code = AsmCodeNew(line);	// 創立要組譯的指令物件
		if (code == NULL)		//如果這行空白就跳過
			continue;
		code->address = address;	// 定義這個指令物件組譯後的地址
		address += code->size;		// 更新下一個指令的初始地址
		if (code->op != NULL){
			Op *op = HashTableGet(opTable, code->op);	// 查詢運算碼是否合法，並找出對應的機器語言與類別
			if (op != NULL){
				code->opCode = op->code;	// 組語轉換為機器語言
				code->type = op->type;		// 組語的類型
			}
		}
		if (code->label != NULL){		// 如果是標記符號則把此標記記錄近SymTable
			HashTablePut(a->symTable, code->label, code);
		}
		code->size = AsmCodeSize(code);	// 記錄此行組合語言轉換為機器語言後需要多少位址
		ArrayAdd(a->codes, code);		// 把紀錄好的組語特徵(運算碼或是標記符號)儲存進指令物件array
		AsmCodePrintln(code);			// print出來
	}
	ArrayFree(lines, strFree);		// 釋放讀近來並且分割成array的組語
}

AsmCode* AsmCodeNew(char *line){
	AsmCode* code = ObjNew(AsmCode, 1);		// 創建空AsmCode object
	Array* tokens = split(line, " []\t,+", REMOVE_SPLITER);		// 因為要考慮[]與+會使組譯器更加複雜，目前沒有加入這部分功能
	if (!tokens->count){		// 若讀進的line為空
		ArrayFree(tokens, strFree);		// 釋放記憶體並return
		return;
	}
	strTrim(line, line, " \t\n\r\t");	// 修剪code text頭尾把空格換行等部分刪去
	code->line = newStr(line);		// 創建一個新的code text避免被釋放，並儲存進asmCode
	code->tokens = tokens;			// 把分割成array的tokens存進asmCode
	int tokenIdx = 0;
	if (strTail(tokens->item[0], ':')){	//判斷是否是標記符號
		code->label = tokens->item[tokenIdx++];		// 把標記存進AsmCode
		strTrim(code->label, code->label, ":");		// 去除':'
	}
	else{
		code->label = NULL;		// 若非標記符號則設為空
	}
	code->op = ArrayGet(tokens, tokenIdx++);	// 若有運算符則儲存進AsmCode，若無則為NULL
	code->argStart = tokenIdx;	// 紀錄arg開始位置
	code->argNum = 0;			// 紀錄總共有幾個arg
	code->opCode = OP_NULL;		// 初始化opCode
	for (int i=0; i<3; i++){	// 遍歷儲存args
		code->arg[i] = ArrayGet(tokens, tokenIdx++);
		if (code->arg[i]!=NULL){
			code->argNum = i+1;
		}
	}
	AsmCodePrintln(code);
	return code;
}

int AsmCodeSize(AsmCode* code){
	Array *tokens = code->tokens;
	
	switch (code->opCode){
		case OP_RESW:
			if (code->argStart < tokens->count){
				return 4*atoi(tokens->item[code->argStart]);
			}
			return 0;
		case OP_RESB:
			if (code->argStart < tokens->count){
				return atoi(tokens->item[code->argStart]);
			}
			return 0;
		case OP_WORD:
			return 4* (code->argNum+1);
		case OP_BYTE:
			return code->argNum+1;
		case OP_NULL:
			return 0;
		default:
			return 4;
	}
}

void AsmPass2(Assembler *a){
	printf("---------------------PASS 2----------------------\n");
	for (int i=0; i<a->codes->count; i++){
		AsmCode *code = a->codes->item[i];	// 讀取asmcode
		AsmTranslateCode(a, code);			// translate to machine code
		AsmCodePrintln(code);
	}
}

void AsmTranslateCode(Assembler *a, AsmCode *code){       
	code->objCode = newMemory(code->size*2+1);		// 總共需要code->size大小的bytes,*2是因為malloc以字結為單位,並且+1結尾
	memset(code->objCode, '\0', code->size*2+1);	// 初始化
	char cxCode[9]="00000000";
	switch (code->type){
		case 'J':
			AsmCodeParse(code, a);
			sprintf(cxCode, "%8x", code->cx);
      		sprintf(code->objCode, "%2x%s", code->opCode, &cxCode[2]);
			break;
		case 'L':
			AsmCodeParse(code, a);
			sprintf(cxCode, "%8x", code->cx);
			sprintf(code->objCode, "%2x%x%x%s", code->opCode, code->r[0], code->r[1], &cxCode[4]);
			break;
		case 'A':
			AsmCodeParse(code, a);
			sprintf(cxCode, "%8x", code->cx);
			sprintf(code->objCode, "%2x%x%x%x%s", code->opCode,code->r[0],code->r[1],code->r[2],&cxCode[5]);
			break;
		case 'D':
			AsmCodeParseD(code, a);
	}
	strReplace(code->objCode, " ", '0');
	strToUpper(code->objCode);
}

void AsmCodeParseD(AsmCode* code, Assembler *a){
	Array *tokens = code->tokens;
	char format4[]="%8x", format1[]="%2x", *format = format1;
	switch (code->opCode){
		case OP_RESW:
		case OP_RESB:
			memset(code->objCode, '0', code->size*2);
			break;
		case OP_WORD:
			format = format4;
		case OP_BYTE:
			char *objPtr = code->objCode;
			for (int i=code->argStart; i<tokens->count; i++){
				char *item = tokens->item[i];
				if (isdigit(item[0]))
					sprintf(objPtr, format, atoi(item));
				else{
					AsmCode *itemCode = HashTableGet(a->symTable, item);
					sprintf(objPtr, format, itemCode->address);
				}
				strFree(item);
				objPtr += strlen(objPtr);
			}
			break;
		default:
			break;
	}
}

void AsmCodeParse(AsmCode* code, Assembler *a){
	int i=0;
	while (code->arg[i]!=NULL && i<3){				// 遍歷args
		if (code->arg[i][0]=='R'){					// 若是arg是暫存器，即開頭為R
			sscanf(code->arg[i], "R%d", &code->r[i++]);	// 因暫存器僅占1~15，因此轉換為地址時僅有數字即可
		}
		else{
			sscanf(code->arg[i], "%d", &code->cx);	// 若開頭並非R則代表此arg是常數或是記錄在符號表
			AsmCode *label = HashTableGet(a->symTable, code->arg[i]);
			if (label != NULL){				// 若是為常數則位置不用修改，若是符號則需照符號表與pass1紀錄的地址計算相對於pc(15)的位置
				code->cx = label->address - code->address - 4;
				code->r[i++] = 15;
			}
		}
	}
	code->rCount = i;
}


void AsmSaveObjFile(Assembler *a, char *objFile){
	printf("---------------------SAVE OBJFILE----------------------\n");
	FILE *file = fopen(objFile, "wb");
	for (int i=0; i<a->codes->count; i++){
		AsmCode *code = a->codes->item[i];
		char *objPtr = code->objCode;
		while (*objPtr != '\0'){
			int x;
			sscanf(objPtr, "%2x", &x);
			assert(x >= 0 && x < 256);
			BYTE b = (BYTE) x;
			fwrite(&b, sizeof(BYTE), 1, file);
			objPtr += 2;
			char bstr[3];
			sprintf(bstr, "%2x", b);
			strReplace(bstr, " ", '0');
			strToUpper(bstr);
			printf("%s", bstr);
		}
	}
	printf("\n");
  	fclose(file);
}

void AsmCodeFree(AsmCode *code) {
	freeMemory(code->line);
	ArrayFree(code->tokens, strFree);
	freeMemory(code->objCode);
	freeMemory(code);
}

void AsmFree(Assembler *a) {
	ArrayFree(a->codes, (FuncPtr1) AsmCodeFree);
	HashTableFree(a->symTable);
	OpTableFree();
	ObjFree(a);
}