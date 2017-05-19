#include <stdio.h>
#include <string.h>



int main(int argc, char ** argv) {

  int request_lba1 = 0, request_lba2 = 0, sectors = 0, pages = 0, RAs = 0, temp = 0, ret1 = 0, ret2 = 0;

  ret1 = scanf("%d", &request_lba1);
  ret2 = scanf("%d", &sectors);

  while (ret1 && ret2 && sectors && request_lba1) {
    temp = sectors*512;
    ret1 = scanf("%d", &request_lba2);
    ret2 = scanf("%d", &sectors); 

    while (request_lba1 + temp == request_lba2 && ret1 && ret2) {
      temp += sectors*512;
      ret1 = scanf("%d", &request_lba2);
      ret2 = scanf("%d", &sectors);
      if (!request_lba2 || !sectors)
        break;
    }

    pages += temp/4096 + ((temp%4096)? 1: 0);
    RAs++;
    request_lba1 = request_lba2;

  } while (ret1 && ret2 && sectors && request_lba1);
  
  printf("\n4k blocks read:\t\t %d\n4k blocks read sequentially:\t %d\nLayout Score:\t\t %f\n", pages, pages - RAs, 1.0*(pages - RAs)/pages);

  return 0;

}
