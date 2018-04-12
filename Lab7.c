/*  ECE 587 Lab 7: Memory Simulator
 *
 *      Simulates Memory.
 *
 *  Author: Nolan Kitchin
 *    CWID: 11572421
 *    Date: 26 March 2018
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>

int mainMem;
int cacheSize;
int cacheBlockSize;
int setAssoc;
char replPolicy[100];
char fileName[100];
FILE *inputFile;

//May need to fix scope later:
int cacheBlkNum;
int bitTag;
int totalHits;
int numMemRef;
int highestHits;
// int cacheBlks[];
// int dirtyBits[];
// int validBits[];

typedef struct {
  int num;
  int dirtyBit;
  int validBit;
  int count;
  int tag;
  char data[100];
} memBlock;

memBlock cacheBlock[16384];

int isNumeric(char* str){
  for(int i = 0; i < strlen(str); i++){
    if(str[i] == '\n'){
      continue;
    }
    if(!isdigit(str[i])){
      return 0;
    }
  }
  return 1;
}

int intLog2(int base2Num){
  if(base2Num % 2 != 0){
    printf("Error. Only Base 2 numbers or 1 is allowed.\n");
    return -1;
  }else if(base2Num == 1){
    return 0;
  }
  int count = 0;
  while(base2Num > 1){
    base2Num /= 2;
    count++;
  }
  return count;
}

char *extractBinTag(int num){
  static char temp[20];
  strcpy(temp,"");
  if(num == 0){
    for(int i = 0; i < bitTag; i++){
      strcat(temp,"0");
    }
    return temp;
  }else if(num == -1){
    for(int i = 0; i < bitTag; i++){
      strcat(temp,"x");
    }
    return temp;
  }
  int rem;
  while(num > 0){
    rem = num % 2;
    num /= 2;
    if(rem == 0){
      strcat(temp, "0");
    }else{
      strcat(temp, "1");
    }
  }

  while(strlen(temp) < intLog2(mainMem)){
    strcat(temp,"0");
  }

  char revStr[20];
  strcpy(revStr, strrev(temp));

  static char tagStr[20];
  strcpy(tagStr, "");
  // for(int i = 0; i < bitTag; i++){
  //   strcat(tagStr, revStr[i]);
  // }
  strncat(tagStr, revStr, bitTag);

  // printf("tag = {%d}", cacheBlock[0].tag);
  // printf("tagStr = |%s|", tagStr);
  // printf("temp = %d",strlen(temp));

  return tagStr;
}

char *tagBin(int num){
  static char temp[20];
  strcpy(temp,"");

  if(num == 0){
    while(strlen(temp) < bitTag){
      strcat(temp, "0");
    }
  }else if(num == -1){
    while(strlen(temp) < bitTag){
      strcat(temp, "x");
    }
    return temp;
  }

  int rem;
  while(num > 0){
    rem = num % 2;
    num /= 2;
    if(rem == 0){
      strcat(temp, "0");
    }else{
      strcat(temp, "1");
    }
  }
  while(strlen(temp) < bitTag){
    strcat(temp, "0");
  }

  static char revStr[20];
  strcpy(revStr, strrev(temp));
  return revStr;
}

// char *tagBin(int num){
//   static char temp[20];
//   strcpy(temp,"");
//   if(num == 0){
//     for(int i = 0; i < bitTag; i++){
//       strcat(temp,"0");
//     }
//     return temp;
//   }else if(num == -1){
//     for(int i = 0; i < bitTag; i++){
//       strcat(temp,"x");
//     }
//     return temp;
//   }
//   int rem;
//   while(num > 0){
//     rem = num % 2;
//     num /= 2;
//     if(rem == 0){
//       strcat(temp, "0");
//     }else{
//       strcat(temp, "1");
//     }
//   }
//
//   while(strlen(temp) < bitTag){
//     strcat(temp,"0");
//   }
//
//   char revStr[20];
//   strcpy(revStr, strrev(temp));
//
//   return revStr;
// }

int tagInt(char * tagString){
  int num = 0;
  char inter[100];
  strcpy(inter, tagString);
  char reversed[100];
  strcpy(reversed, strrev(inter));
  for(int i = 0; i < strlen(reversed); i++){
    num += pow(2.0, i) * (reversed[i]-48);
  }
  return num;
}

void specifyMemorySpecs(){
  for( ; ; ){
    printf("Enter the size of main memory in bytes: ");
    char memStr[100];
    scanf("%s",memStr);
    if(isNumeric(memStr)){
      mainMem = atoi(memStr);
      if(mainMem < 4 || mainMem > 32768){
        printf("Error! Input out of range. Valid range is 4 to 32768 bytes.\n\n");
      }else{
        break;
      }
    }else{
      printf("Error! Only numeric input is valid.\n\n");
    }
  }

  for( ; ; ){
    printf("Enter the size of the cache in bytes: ");
    char cacheStr[100];
    scanf("%s", cacheStr);
    if(isNumeric(cacheStr)){
      cacheSize = atoi(cacheStr);
      if(cacheSize < 2 || cacheSize >= mainMem){
        printf("Error! Input out of range. Valid range is 2 to %d bytes.\n\n", mainMem-1);
      }else{
        break;
      }
    }else{
      printf("Error! Only numeric input is valid.\n\n");
    }
  }

  for( ; ; ){
    printf("Enter the cache block/line size in bytes: ");
    char blockStr[100];
    scanf("%s", blockStr);
    if(isNumeric(blockStr)){
      cacheBlockSize = atoi(blockStr);
      if(cacheBlockSize < 2 || cacheBlockSize > cacheSize){
        printf("Error! Input out of range. Valid range is 2 to %d bytes.\n\n", cacheSize);
      }else{
        break;
      }
    }else{
      printf("Error! Only numeric input is valid.\n\n");
    }
  }


  // for(int i = 0; i < cacheBlkNum; i++){
  //   cacheBlks[i] = i;
  //   dirtyBits[i] = 0;
  //   validBits[i] = 0;
  // }

  for( ; ; ){
    printf("Enter the degree of set-associativity (input n for an n-way set-associative mapping): ");
    char assocStr[100];
    scanf("%s", assocStr);
    if(isNumeric(assocStr)){
      setAssoc = atoi(assocStr);

      //TODO: fix range on setAssoc
      if(setAssoc < 1 || setAssoc > 32768){
        printf("Error! Input out of range. Valid range is 2 to 32768 bytes.\n\n");
      }else{
        break;
      }
    }else{
      printf("Error! Only numeric input is valid.\n\n");
    }
  }

  for( ; ; ){
    printf("Enter the replacement policy (L = LRU, F = FIFO): ");
    scanf("%s", replPolicy);
    if(strcmp(replPolicy, "L") != 0 && strcmp(replPolicy, "F") != 0){
      printf("\nError! Invalid input. Only 'L' or 'F' are acceptable inputs.\n\n");
    }else{
      break;
    }
  }
  return;
}

void memCalc(){
  int addrLines = intLog2(mainMem);
  int bitOffset = intLog2(cacheBlockSize);
  int bitIndex = intLog2((cacheSize/cacheBlockSize)/setAssoc);
  bitTag = addrLines - (bitOffset + bitIndex);
  int overheadBits;
  int totalCacheSize = cacheSize + (bitTag + 2)*(cacheSize / cacheBlockSize)/8;
  printf("\nSimulator Output:\n");
  printf("Total address lines required = %d\n", addrLines);
  printf("Number of bits for offset = %d\n", bitOffset);
  printf("Number of bits for index = %d\n", bitIndex);
  printf("Number of bits for tag = %d\n", bitTag);
  printf("Total cache size required = %d bytes\n", totalCacheSize);
}

//TODO: figure out how to keep track of cache
int accessMem(int memoryLoc, int refCount, int readOrWrite){
  int mmBlk = memoryLoc / cacheBlockSize;
  int set = mmBlk % (cacheSize / cacheBlockSize / setAssoc);
  int low = set*setAssoc;
  static int high;
  high = low + setAssoc -1;

  //char hitmiss[4];
  //char binTag[100];
  //strcpy(binTag, int2BinStr(set));
  // printf("\n\nset = |%d|", set);
  // printf("\nbinTag = |%s|", binTag);

  // printf("\n\nRANGE: (%d - %d)", low, high);
  int lowestRef = cacheBlock[low].count;
  int indexOfLowest = low;
  for(int i = low; i <= high; i++){
    // printf("\n\nINDEX = %d", i);
    // printf("\nlow = %d", low);
    // printf("\nhigh = %d", high);
    if(cacheBlock[i].count < lowestRef){
      lowestRef = cacheBlock[i].count;
      indexOfLowest = cacheBlock[i].num;
    }
    //int tempTag = cacheBlock[i].tag;
    //printf("\n\n\ntempTag = |%s|", tempTag);
    if(cacheBlock[i].tag == tagInt(extractBinTag(memoryLoc)) && cacheBlock[i].validBit == 1){
      //strcpy(hitmiss, "hit");
      if(strcmp(replPolicy, "L") == 0){
        cacheBlock[i].count = refCount;
      }
      if(readOrWrite == 0){
        cacheBlock[i].dirtyBit = 0;
      }else if(readOrWrite == 1){
        cacheBlock[i].dirtyBit = 1;
      }
      totalHits++;
      return 1;
    }

    if(cacheBlock[i].validBit == 0){
      cacheBlock[i].validBit = 1;
      if(readOrWrite == 0){
        cacheBlock[i].dirtyBit = 0;
      }else if(readOrWrite == 1){
        cacheBlock[i].dirtyBit = 1;
      }
      cacheBlock[i].count = refCount;
      //strcpy(cacheBlock[i].tag, binTag);
      cacheBlock[i].tag = tagInt(extractBinTag(memoryLoc));
      char dataStr[100];
      //sprintf(dataStr, "mm blk #%d | %d", mmBlk, cacheBlock[i].count);
      sprintf(dataStr, "mm blk #%d", mmBlk);
      strcpy(cacheBlock[i].data, dataStr);
      return 0;
    }
  }
  if(readOrWrite == 0){
    cacheBlock[indexOfLowest].dirtyBit = 0;
  }else if(readOrWrite == 1){
    cacheBlock[indexOfLowest].dirtyBit = 1;
  }
  cacheBlock[indexOfLowest].validBit = 1;
  //strcpy(cacheBlock[lowestRef].tag, binTag);
  cacheBlock[indexOfLowest].count = refCount;
  cacheBlock[indexOfLowest].tag = tagInt(extractBinTag(memoryLoc));
  char dataStr[100];
  //sprintf(dataStr, "mm blk #%d | %d", mmBlk, cacheBlock[indexOfLowest].count);
  sprintf(dataStr, "mm blk #%d", mmBlk);
  strcpy(cacheBlock[indexOfLowest].data, dataStr);
  return 0;

  // if(replPolicy == 'F'){
  //   cacheBlock[lowestRef].count = refCount;
  // }else if(replPolicy == 'L'){
  //
  // }
}

void printHeader(){
  printf("\nmain memory address\tmm blk #\tcm set #\tcm blk #\thit/miss\n");
  for(int i = 0; i < 80; i++){
    printf("%c",196);
  }
}

void printReferenceLine(int mmAddr, int found){
  int mmBlkNum = mmAddr / cacheBlockSize;
  int setNum = mmBlkNum % (cacheSize / cacheBlockSize / setAssoc);
  //TODO: figure out how to get hit/miss
  char outcome[5];

  if(found == 1){
    strcpy(outcome, "hit");
  }else if(found == 0){
    strcpy(outcome, "miss");
  }

  if(setAssoc == 1){
    printf("\n%7d%20d%16d%16d%16s",mmAddr,mmBlkNum,setNum,setNum,outcome);
  }else{
    int cmBlkNumLow = setNum*setAssoc;
    int cmBlkNumHigh = cmBlkNumLow + setAssoc - 1;
    printf("\n%7d%20d%16d%16d - %d%16s",mmAddr,mmBlkNum,setNum,cmBlkNumLow,cmBlkNumHigh,outcome);
  }
}

void printHitRates(){
  int hits;
  float percent = (float)totalHits/numMemRef*100;
  printf("\nHighest possible hit rate = %d/%d = %.2f%%\n",hits,numMemRef,percent);
  printf("Actual hit rate = %d/%d = %.2f%%\n",totalHits,numMemRef,percent);
}

void printFinalCache(){
  printf("\nFinal \"status\" of the cache:\n");
  printf("\nCache blk #\tdirty bit\tvalid bit\t  tag\t\tData\n");
  for(int i = 0; i < 80; i++){
    printf("%c",196);
  }

  for(int i = 0; i < cacheBlkNum; i++){
    char dirtyBitStr[5];
    if(cacheBlock[i].dirtyBit == -1){
      strcpy(dirtyBitStr, "x");
    }else{
      sprintf(dirtyBitStr, "%d", cacheBlock[i].dirtyBit);
    }
    printf("\n%7d%14s%16d%16s\t\t%s\n", cacheBlock[i].num, dirtyBitStr, cacheBlock[i].validBit, tagBin(cacheBlock[i].tag), cacheBlock[i].data);
  }
}

void printProgramTitle(){
  printf("\t\t");
  for(int i = 0; i < 70; i++){
    printf("%c",176);
  }
  printf("\n\t\t%c%c%67c%c\n\t\t%c%c%51s%16c%c\n\t\t%c%c%67c%c\n\t\t",176,176,176,176,176,176,"M E M O R Y   S I M U L A T O R   1.0", 176,176,176,176,176,176);
  //TODO: Make width a variable. Add Class, Lab, Name?
  for(int i = 0; i < 70; i++){
    printf("%c",176);
  }
  printf("\n\n");
}

int parseFile(){
  int freePosition = 0;
  printf("Enter the name of the input file containing the list of memory references generated by the CPU: ");
  scanf("%s", fileName);
  errno = 0;
  inputFile = fopen(fileName, "r");
  if(errno == 2){
    perror("\n\nError");
    printf("\n\n");
    errno = 0;
    return 0;
  }else if(errno == 0){
    char memRefStr[100];
    fscanf(inputFile, "%s", memRefStr);
    fgetc(inputFile);
    char newLine[100];
    fgets(newLine, 100, inputFile);
    if(isNumeric(memRefStr)){
      numMemRef = atoi(memRefStr);
      int memRef[numMemRef];
      if(strcmp(newLine,"\n") != 0){
        printf("\n\nError reading file '%s'.  The second line of the input file must be a carriage return.\n\n\n", fileName);
        return 0;
      }
      int referenceCounter = 0;
      while(fgets(newLine, 100, inputFile) != NULL){
        //printf("%s", newLine);
        if(newLine[0] == 'R' || newLine[0] == 'W'){
          if(!isspace(newLine[1])){
            printf("\n\nError reading file '%s'.  There must be a space after each 'R' or 'W' after the second line of the file.\n\n", fileName);
            return 0;
          }else{
            char restOfLine[100];
            strcpy(restOfLine, newLine + 2);
            if(!isNumeric(restOfLine)){
              printf("\n\nError reading file '%s'.  Memory locations must be numeric.\n\n\n", fileName);
              return 0;
            }else{
              int memLoc = atoi(restOfLine);
              memRef[freePosition] = memLoc;
              freePosition++;
              if(memLoc < 0 || memLoc > mainMem - 1){
                printf("\n\nError reading file '%s'.  %d is out of range.  Valid memory addresses range from 0 to %d \n\n", fileName, memLoc, mainMem-1);
                return 0;
              }else{
                if(referenceCounter == 0){
                  memCalc();
                  printHeader();
                }
                int RW;
                if(newLine[0] == 'R'){
                  RW = 0;
                }else if(newLine[0] == 'W'){
                  RW = 1;
                }
                int success = accessMem(memLoc, referenceCounter, RW);
                printReferenceLine(memLoc, success);
                //printFinalCache();  //UNCOMMENT TO VIEW CACHE AFTER EACH MEMORY ACCESS
              }
            }
          }
        }else{
          printf("\n\nError reading file '%s'.  The first character of each line after the first two lines must be 'R' or 'W'.", fileName);
          return 0;
        }

        if(referenceCounter > numMemRef){
          printf("\n\nError reading file '%s'.  More references in the file than specified by the first line.", fileName);
        }
        referenceCounter++;
      }
      for(int i = 0; i < numMemRef; i++){
        printf("\n\nREF = %d", memRef[i]);
      }
      printf("\n");
      return 1;
    }else{
      printf("\n\nError reading file '%s'.  The first line of the input file must be numeric.\n\n", fileName);
      return 0;
    }
  }
}

int main(){

  printProgramTitle();

  for( ; ; ){

    specifyMemorySpecs();
    cacheBlkNum = cacheSize / cacheBlockSize;
    //struct memBlock cacheBlock[cacheBlkNum];
    int tagBits = intLog2(mainMem) - intLog2(cacheBlockSize) - intLog2((cacheSize/cacheBlockSize)/setAssoc);
    char tempStr[] = "";
    for(int i = 0; i < tagBits; i++){
      strcat(tempStr,"x");
    }
    //printf("%s",tempStr);
    for(int i = 0; i < cacheBlkNum; i++){
      cacheBlock[i].num = i;
      cacheBlock[i].dirtyBit = -1;
      cacheBlock[i].validBit = 0;
      cacheBlock[i].tag = -1;
      strcpy(cacheBlock[i].data,"     X");
      //printf("\n%d\t%d\t%d\t%s\t%s\n", cacheBlock[i].num, cacheBlock[i].dirtyBit, cacheBlock[i].validBit, tagBin(cacheBlock[i].tag), cacheBlock[i].data);
    }

    // struct memBlock *memPtr;
    // memPtr = &cacheBlock;

    //printf("%d", memPtr[0].num);
    for( ; ; ){
      int fileReadSuccess = parseFile();
      if(fileReadSuccess){
        break;
      }
    }

    printHitRates();

    printFinalCache();

    char cont[100];
    for( ; ; ){
      printf("\nContinue? (y = yes, n = no): ");
      scanf("%s", cont);
      printf("\n");
      if(strcmp(cont, "y") != 0 && strcmp(cont, "n") != 0){
        printf("\nError! Please only enter 'y' or 'n'.\n\n");
      }else{
        break;
      }
    }

    if(strcmp(cont, "n") == 0){
      break;
    }

  }
  return 0;
}
