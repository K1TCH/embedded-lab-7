#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

int intLog2(int base2Num){
  if(base2Num == 1){
    return 0;
  }else if(base2Num % 2 != 0){
    printf("Error. Only Base 2 numbers allowed.\n");
    return -1;
  }
  int count = 0;
  while(base2Num > 1){
    base2Num /= 2;
    count++;
  }

  if(base2Num == 0){
    return count;
  }
  return -1;
}

int bitTag = 3;

char *int2BinStr(int num){
  char temp[20];
  strcpy(temp,"");
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

FILE *fp;
int main(){

  // printf("log2(%d) = %d\n", 1, intLog2(1));
  // for(int x = 2; x < 1024; x*=2){
  //   printf("log2(%d) = %d\n", x, intLog2(x));
  // }
  // intLog2(3);

  // fp = fopen("input_file1.txt", "r");
  // char str[100];
  // while(fgets(str,100,fp) != NULL){
  //   printf("%s",str);
  // }

  // char str1[] = "hello";
  // char str2[100];
  // memcpy(str2, str1 + 1, sizeof(str1));
  // printf("%s", str2);
  // char x[100];
  // strcpy(x, int2BinStr(2));
  // printf("%s",x);
  // char y[100];
  // strcpy(y, int2BinStr(347));
  // printf("%s", y);
  //printf("%d", tagInt("101"));
  //printf("%s", int2BinStr(2));
  printf("%d", intLog2(16382));

}
