#include "machineCode.h"
#include "symbolTable.h"

static int codeArg(char *arg, int AddMethod, int IC);

/* changing the instAddressType to 0,1,2,3  instead of 1,2,4,8 will destroy the gentle logic of the the legalTable you have built - yair */
int calculAddress(int AddressType){

	switch (AddressType) {
		case NON:
			return 0;
		case IMM:
			return 0;
		case DIR:
			return 1;
		case REL:
			return 2;
		case REG:
			return 3;	
	}

	return -1;
}

/* codeInstruction - codes an instruction into the instrcution image
 * INPUT: instrcution opfunct (contains the opcode and funct of the commands), args and their addressing methods
 * OUTPUT: number of words to add to instrctuion counter
 * METHOD:  complete this later on
 */
int buildBinaryCode(int opfunct, char* srcOper, char* destOper, int sourceAdd, int destAdd, int IC) {

	int numOfWords = 1;

	/*coding the first word the instrcution it self */
	instIamge[IC].conent.head.opCode = (unsigned int) (opfunct / NUM_CMD); /* gets all the necessary data for the coding */
	instIamge[IC].conent.head.funct = (unsigned int) (opfunct % NUM_CMD);
	instIamge[IC].conent.head.srcAdress = (unsigned int) calculAddress (sourceAdd );
	instIamge[IC].conent.head.destAdress = (unsigned int) calculAddress (destAdd );
	instIamge[IC].conent.head.srcReg = (unsigned int) (sourceAdd == REG) ? (srcOper[1] - '0') : 0;
	instIamge[IC].conent.head.destReg = (unsigned int) (destAdd == REG) ? (destOper[1] - '0') : 0;
	instIamge[IC].conent.head.A = 1; /* in the first word of every instruction A = 1 */
	instIamge[IC].conent.head.E = 0; /* in the first word of every instruction E = 0 */
	instIamge[IC].conent.head.R = 0; /* in the first word of every instruction R = 0 */
	instIamge[IC].conent.head.lineNumber = lineNumber;
	instIamge[IC].type = HEADER_LINE;

	numOfWords += codeArg(srcOper, sourceAdd, IC + 1); /* word next to IC */
	numOfWords += codeArg(destOper, destAdd, IC + 1 + (numOfWords == 2)); /* word next to word next to IC */

	return numOfWords; /* number of words added to code image */
}

static int codeArg(char *arg, int AddMethod, int IC) {

	switch (AddMethod) {
	case NON:
	case REG:
		return 0; /* i.e. no word of data added */
	case IMM:
		instIamge[IC].conent.value.data = atoi(++arg); /* צריך לבדוק שזה מספר!!!  וגם שזה עומד בגדלים הנדרשים*/
		instIamge[IC].conent.value.labelName = NULL;
		instIamge[IC].conent.value.A = 1;
		instIamge[IC].conent.value.R = 0;
		instIamge[IC].conent.value.E = 0;
		break;

	case DIR:
		if ( isSymbolExternal(arg) > 0){	/* if the symbol is external */
			instIamge[IC].conent.value.data = 0; 
			instIamge[IC].conent.value.labelName = calloc(strlen(arg)+1, sizeof(char));
			strcpy(instIamge[IC].conent.value.labelName, arg);
			instIamge[IC].conent.value.A = 0;
			instIamge[IC].conent.value.R = 0;
			instIamge[IC].conent.value.E = 1;
		}
		else { /* symbol is not external */
			instIamge[IC].conent.value.data = 0; 
			instIamge[IC].conent.value.labelName = calloc(strlen(arg)+1, sizeof(char));
			strcpy(instIamge[IC].conent.value.labelName, arg);
			instIamge[IC].conent.value.A = 0;
			instIamge[IC].conent.value.R = 1;
			instIamge[IC].conent.value.E = 0;
		}
		break;
	case REL:
		instIamge[IC].conent.value.data = 0;
		instIamge[IC].conent.value.labelName = calloc(strlen(arg)+1, sizeof(char));
		strcpy(instIamge[IC].conent.value.labelName, ++arg); /* skip the '&' in relative addressing */
		instIamge[IC].conent.value.A = 1;
		instIamge[IC].conent.value.R = 0;
		instIamge[IC].conent.value.E = 0;
		break;
	}

	instIamge[IC].type = DATA_LINE;
	return 1; /* i.e. a word of data was added */
}

int getLineType(int IC) {
	return instIamge[IC].type;
}

char *getSymbol(int IC) {
	return instIamge[IC].conent.value.labelName;
}

int getLineNumber(int IC) {
	return instIamge[IC].conent.head.lineNumber;
}

void setSymbolValue(int IC, int lastCommandIC, int symbolValue) {
	if (instIamge[IC].conent.value.A)  /* i.e. the addressing method is REL */
		instIamge[IC].conent.value.data = (symbolValue - lastCommandIC - MMRY_OFFSET) ;	/*100 is the memoroffset */

	else  /* i.e. addressing method is DIR */
		instIamge[IC].conent.value.data = symbolValue;

}

/* set symbol to be external */
void setExternSymbol(int IC) {
	instIamge[IC].conent.value.E = 1;
	instIamge[IC].conent.value.R = 0;
	instIamge[IC].conent.value.A = 0;
}





/* after testing that everything is working fine, move all the function bellow to filesBuilder.c */

/* transforms 32 number to his 24 bit equvelent */
int bitTobit (int val) {

    	int max24 = 0x7FFFFF;  /* limit to 23 bits beacuse thers is the  leading sign bit  */
	int mask = 0x00FFFFFF;

    	if ( val >= 0 && val < max24 ) /* positive number smaller than 2^23   */
       		return val;

   	else if (val < 0 && val >= (-1*max24) )  /* the logic behind this is to trasfer the 8 leading bits to zero's */
        	return (val&mask) ; 

	else {
		printf("warning in %d: the number :%d is cannot be written in 24 bits format\n", lineNumber, val);
		return -1;
	}
}

/* parse line to integer */	
 int parseLineToNum( line ln ) {

	int num = 0;

	if ( ln.type == HEADER_LINE ) {
		num += ln.conent.head.E ;
		num += ln.conent.head.R<<1 ;
		num += ln.conent.head.A<<2 ;
		num += ln.conent.head.funct<<3 ;
		num += ln.conent.head.destReg<<8 ;
		num += ln.conent.head.destAdress<<11 ;
		num += ln.conent.head.srcReg<<13 ;
		num += ln.conent.head.srcAdress<<16;
		num += ln.conent.head.opCode<<18;
	}

	else if (ln.type == DATA_LINE) { 
		num += ln.conent.value.E ;
    		num += ln.conent.value.R<<1 ;
   		num += ln.conent.value.A<<2 ;
   		num += ln.conent.value.data<<3 ;
	}

	/* transforms 32 number to his 24 bit equvelent */
	return bitTobit (num);
} 

/*parse dataNode which is string or integers into machine code */
int parseData (dataPtr ptr, int type, int offSet, int length) {

	int  i; /* the length of the array */	
	
	if (type == DATA) { /* integers */		
		for (i=0 ; i < length ; i++  ) 
			printf ("%06d %06x\n" , 100+offSet+i , bitTobit ( ptr.intPtr[i] ) ) ;
	}
	
	else if (type == STR) { /* string  */		
		for (i=0 ; i < length ; i++  ) 
			printf ("%06d %06x\n" , 100+offSet+i , ptr.strPtr[i] ) ;
	}
	return length;
}

int printImage( int ICF, dataNode dataImage) {

	int i ;
	dataNode* dataImagePtr = &dataImage;
	
	for (i=0 ; i < ICF-100 ; i++ ) 
		printf("%06d %06x\n" , i+100, parseLineToNum (instIamge[i]) ) ; 

	while( dataImagePtr ) {
		i += parseData (dataImagePtr->data, dataImagePtr->type , i ,dataImagePtr->length  ) ;
		dataImagePtr = dataImagePtr->next;
	}
	

	return 1;
}
