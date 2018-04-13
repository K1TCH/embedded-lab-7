/*  ECE 587 Lab 7: Memory Simulator
 *
 *  Author: Nolan Kitchin
 *    CWID: 11572421
 *    Date: 26 March 2018
 */

// Standard Libraries necessary
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>

int mainMem;            // size of the main memory
int cacheSize;          // size of the cache memory
int cacheBlockSize;     // size of each block of memory
int setAssoc;           // degree of set-associativity
char replPolicy[100];   // string to hold the replacement policy user input
char fileName[100];     // string to hold the file name user input
FILE *inputFile;        // pointer to the input file

int cacheBlkNum;        // total number of cache blocks
int bitTag;             // number of bits in the tag field
int totalHits;          // counter for the hit rate
int numMemRef;          // number of memory references specified by the file
int highestHits;        // count for the highest possible hit rate

// Structure to hold information on a single cache block
typedef struct {
  int num;              // index of the cache block
  int dirtyBit;         // dirty bit value
  int validBit;         // valid bit value
  int count;            // count for implementing LRU and FIFO
  int tag;              // integer tag number
  char data[100];       // data to hold the main memory block number
} memBlock;

// allocate for the maximum scenario
memBlock cacheBlock[16384];

// determines if a string is completely numeric for error-checking
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

// implementation of log base 2 without using math.h
int intLog2(int base2Num){

  if(base2Num == 1){
    return 0;
  }else if(base2Num % 2 != 0){
    return -1;
  }

  int count = 0;
  while(base2Num > 1){
    if(base2Num % 2 != 0){
      return -1;
    }
    base2Num /= 2;
    count++;
  }

  return count;
}

// extracts binary string representing the tag value of a memory reference
char *extractBinTag(int num){
  static char temp[20];
  strcpy(temp,"");
  if(num == 0){
    for(int i = 0; i < bitTag; i++){
      strcat(temp,"0");
    }
    return temp;
  }else if(num == -1){                      // a tag of -1 is a don't care
    for(int i = 0; i < bitTag; i++){
      strcat(temp,"x");
    }
    return temp;
  }
  int rem;
  while(num > 0){                           // binary conversion
    rem = num % 2;
    num /= 2;
    if(rem == 0){
      strcat(temp, "0");
    }else{
      strcat(temp, "1");
    }
  }

  // pad with zeros
  while(strlen(temp) < intLog2(mainMem)){
    strcat(temp,"0");
  }

  // reverse the string
  char revStr[20];
  strcpy(revStr, strrev(temp));

  // only take the most significant bits for the tag
  static char tagStr[20];
  strcpy(tagStr, "");
  strncat(tagStr, revStr, bitTag);

  return tagStr;
}

// converts numeric tag into a binary string for printing
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

// returns the numeric value of a binary tag string
int tagInt(char * tagString){
  int num = 0;
  char inter[100];
  strcpy(inter, tagString);
  char reversed[100];
  strcpy(reversed, strrev(inter));
  for(int i = 0; i < strlen(reversed); i++){
    num += pow(2.0, i) * (reversed[i]-48);      // binary to decimal conversion
  }
  return num;
}

// handle user input of memory specifications
void specifyMemorySpecs(){

  // main memory size
  for( ; ; ){
    printf("Enter the size of main memory in bytes: ");
    char memStr[100];
    scanf("%s",memStr);
    if(strcmp(memStr, "exit") == 0){
      exit(0);
    }
    if(isNumeric(memStr)){
      mainMem = atoi(memStr);
      if(mainMem < 4 || mainMem > 32768){
        printf("Error! Input out of range. Valid range is 4 to 32768 bytes.\n\n");
      }else if(intLog2(mainMem) == -1){
        printf("Error! Input must be a power of 2.\n\n");
      }else{
        break;
      }
    }else{
      printf("Error! Only numeric input is valid.\n\n");
    }
  }

  // cache memory size
  for( ; ; ){
    printf("Enter the size of the cache in bytes: ");
    char cacheStr[100];
    scanf("%s", cacheStr);
    if(strcmp(cacheStr, "exit") == 0){
      exit(0);
    }
    if(isNumeric(cacheStr)){
      cacheSize = atoi(cacheStr);
      if(cacheSize < 2 || cacheSize >= mainMem){
        printf("Error! Input out of range. Valid range is 2 to %d bytes.\n\n", mainMem-1);
      }else if(intLog2(cacheSize) == -1){
        printf("Error! Input must be a power of 2.\n\n");
      }else{
        break;
      }
    }else{
      printf("Error! Only numeric input is valid.\n\n");
    }
  }

  // individual block size
  for( ; ; ){
    printf("Enter the cache block/line size in bytes: ");
    char blockStr[100];
    scanf("%s", blockStr);
    if(strcmp(blockStr, "exit") == 0){
      exit(0);
    }
    if(isNumeric(blockStr)){
      cacheBlockSize = atoi(blockStr);
      if(cacheBlockSize < 2 || cacheBlockSize > cacheSize){
        printf("Error! Input out of range. Valid range is 2 to %d bytes.\n\n", cacheSize);
      }else if(intLog2(cacheBlockSize) == -1){
        printf("Error! Input must be a power of 2.\n\n");
      }else{
        break;
      }
    }else{
      printf("Error! Only numeric input is valid.\n\n");
    }
  }

  // degree of set-associativity
  for( ; ; ){
    printf("Enter the degree of set-associativity (input n for an n-way set-associative mapping): ");
    char assocStr[100];
    scanf("%s", assocStr);

    if(strcmp(assocStr, "exit") == 0){
      exit(0);
    }

    if(strcmp(assocStr, "n") == 0){
      setAssoc = cacheSize / cacheBlockSize;
    }

    if(isNumeric(assocStr) || setAssoc == cacheSize/cacheBlockSize){
      if(setAssoc != cacheSize/cacheBlockSize){
        setAssoc = atoi(assocStr);
      }

      if(setAssoc < 1 || setAssoc > cacheSize / cacheBlockSize){
        printf("Error! Input out of range. Valid range is 1 to %d\n\n", cacheSize / cacheBlockSize);
      }else if(intLog2(setAssoc) == -1){
        printf("Error! Input must be 1, or a power of 2.\n\n");
      }else{
        break;
      }
    }else{
      printf("Error! Only numeric input is valid.\n\n");
    }
  }

  // replacement policy
  for( ; ; ){
    printf("Enter the replacement policy (L = LRU, F = FIFO): ");
    scanf("%s", replPolicy);
    if(strcmp(replPolicy, "exit") == 0){
      exit(0);
    }
    if(strcmp(replPolicy, "L") != 0 && strcmp(replPolicy, "F") != 0){
      printf("\nError! Invalid input. Only 'L' or 'F' are acceptable inputs.\n\n");
    }else{
      break;
    }
  }
  return;
}

// makes and prints calculationns of address lines, number of offset bits,
// number of index bits, number of tag bits,
// as well as the total cache size required
void memCalc(){
  int addrLines = intLog2(mainMem);
  int bitOffset = intLog2(cacheBlockSize);
  int bitIndex = intLog2((cacheSize/cacheBlockSize)/setAssoc);
  bitTag = addrLines - (bitOffset + bitIndex);
  int overheadBits = (bitTag + 2)*(cacheSize/cacheBlockSize);
  int totalCacheSize;
  printf("\nSimulator Output:\n");
  printf("Total address lines required = %d\n", addrLines);
  printf("Number of bits for offset = %d\n", bitOffset);
  printf("Number of bits for index = %d\n", bitIndex);
  printf("Number of bits for tag = %d\n", bitTag);
  if(overheadBits % 8 == 0){
    totalCacheSize = cacheSize + overheadBits/8;
    printf("Total cache size required = %d bytes\n", totalCacheSize);
  }else{
    totalCacheSize = cacheSize + overheadBits / 8;
    overheadBits = overheadBits % 8;
    printf("Total cache size required = %d bytes and %d bits\n", totalCacheSize, overheadBits);
  }

}

// function for handling a read or write of a memory location
int accessMem(int memoryLoc, int refCount, int readOrWrite){
  int mmBlk = memoryLoc / cacheBlockSize;
  int set = mmBlk % (cacheSize / cacheBlockSize / setAssoc);
  int low = set*setAssoc;
  static int high;
  high = low + setAssoc -1;

  int lowestRef = cacheBlock[low].count;
  int indexOfLowest = low;
  for(int i = low; i <= high; i++){
    if(cacheBlock[i].count < lowestRef){
      lowestRef = cacheBlock[i].count;
      indexOfLowest = cacheBlock[i].num;
    }
    if(cacheBlock[i].tag == tagInt(extractBinTag(memoryLoc)) && cacheBlock[i].validBit == 1){
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
      cacheBlock[i].tag = tagInt(extractBinTag(memoryLoc));
      char dataStr[100];
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
  cacheBlock[indexOfLowest].count = refCount;
  cacheBlock[indexOfLowest].tag = tagInt(extractBinTag(memoryLoc));
  char dataStr[100];
  sprintf(dataStr, "mm blk #%d", mmBlk);
  strcpy(cacheBlock[indexOfLowest].data, dataStr);
  return 0;
}

// prints the header of the reference table
void printHeader(){
  printf("\nmain memory address\tmm blk #\tcm set #\tcm blk #\thit/miss\n");
  for(int i = 0; i < 80; i++){
    printf("%c",196);
  }
}

// prints a line of the reference table
void printReferenceLine(int mmAddr, int found){
  int mmBlkNum = mmAddr / cacheBlockSize;
  int setNum = mmBlkNum % (cacheSize / cacheBlockSize / setAssoc);
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

// prints the best possible hit rate and the
// actual hit rate for a given set of memory accesses.
void printHitRates(){
  int hits;
  float highestPercent = (float)highestHits / numMemRef*100;
  float actualPercent = (float)totalHits/numMemRef*100;
  printf("\nHighest possible hit rate = %d/%d = %.2f%%\n",highestHits,numMemRef,highestPercent);
  printf("Actual hit rate = %d/%d = %.2f%%\n",totalHits,numMemRef,actualPercent);
}

// prints the final state of the cache after the entire set of memory references
// has been completed.
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

// prints the title of the program
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
  printf("\t\t  Note: The program can be exited at any prompt by entering 'exit'\n\n");
}

// asks the user for the input file, parses it, checks for errors, and if all is
// successful, it executes the memory references.
int parseFile(){
  int freePosition = 0;
  printf("Enter the name of the input file containing the list of memory references generated by the CPU: ");
  scanf("%s", fileName);
  if(strcmp(fileName, "exit") == 0){
    exit(0);
  }
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
          printf("\n\nError reading file '%s'.  The first character of each line after the first two lines must be 'R' or 'W'.\n\n", fileName);
          return 0;
        }

        referenceCounter++;
        if(referenceCounter > numMemRef){
          printf("\n\nError reading file '%s'.  More references in the file than specified by the first line.\n\n", fileName);
          system("clear");
          return 0;
        }
      }

      if(referenceCounter < numMemRef){
        printf("\n\nError reading file '%s'.  Less references in the file than specified by the first line.\n\n", fileName);
        return 0;
      }
      int mmBlockRef[numMemRef];
      for(int i = 0; i < numMemRef; i++){
        mmBlockRef[i] = memRef[i] / cacheBlockSize;
      }

      highestHits = 0;

      for(int i = 0; i < numMemRef; i++){
        int dontCount = 0;
        for(int k = i-1; k >= 0; k--){
          if(mmBlockRef[i] == mmBlockRef[k]){
            dontCount = 1;
            break;
          }
        }

        if(dontCount == 1){
          continue;
        }

        for(int j = i+1; j < numMemRef; j++){
          if(mmBlockRef[i] == mmBlockRef[j]){
            highestHits++;
          }
        }
      }
      printf("\n");
      return 1;
    }else{
      printf("\n\nError reading file '%s'.  The first line of the input file must be numeric.\n\n", fileName);
      return 0;
    }
  }
}

// the main function
int main(){

  // print the title of the program
  printProgramTitle();

  for( ; ; ){

    // get memory specs from the user
    specifyMemorySpecs();

    // initialize the number of cache blocks specified
    cacheBlkNum = cacheSize / cacheBlockSize;
    int tagBits = intLog2(mainMem) - intLog2(cacheBlockSize) - intLog2((cacheSize/cacheBlockSize)/setAssoc);
    char tempStr[] = "";

    for(int i = 0; i < tagBits; i++){
      strcat(tempStr,"x");
    }

    // parse through the file and make memory accesses
    for( ; ; ){
      totalHits = 0;
      for(int i = 0; i < cacheBlkNum; i++){
        cacheBlock[i].num = i;
        cacheBlock[i].dirtyBit = -1;
        cacheBlock[i].validBit = 0;
        cacheBlock[i].tag = -1;
        strcpy(cacheBlock[i].data,"     X");
      }
      int fileReadSuccess = parseFile();
      if(fileReadSuccess){
        break;
      }
    }

    // print the best possible and actual hit rates
    printHitRates();

    // print the final status of the cache
    printFinalCache();

    // ask the user if they want to start the program over again
    // if not, exit.
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
