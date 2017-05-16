
/* 

The goal of this test is to measure differences in performance on
a partition that is at capacity versus one that has plenty of space.

*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


// counters and settings
static int branch_factor;
static int dir_count;
static int mem_count;
static int mem_lim;
static int file_count;
static int dir_lim;
static int file_size_max;
static int file_size_min;
static int used[300000];
static short int FULL;
static short int copy;
static short int write_test;
static char * file_sys;
static char * partX;
static char * partY;


// tree struct to keep track of the directories being made
typedef struct dir {

  char name[5];
  struct dir ** subdir;
  int subdirs;
  int files;

} dir;


// function to genertate a new node in the directory tree and in the file systems
dir * make_dir(char * path) {

  dir * new = malloc(sizeof(dir));

  int x = dir_count;
  new->name[4] = '\0';
  new->name[3] = x%26 + 'A';
  x /= 26;
  new->name[2] = x%26 + 'A';
  x /= 26;
  new->name[1] = x%26 + 'A';
  new->name[0] = '/';

  new->subdir = calloc(branch_factor, sizeof(dir *));
  new->subdirs = 0;

  new->files = 0;

  dir_count++;

  char buff[1000];
  sprintf(buff, "mkdir /mnt/X%s%s", path, new->name);
  system(buff);
  sprintf(buff, "mkdir /mnt/Y%s%s", path, new->name);
  system(buff);

  return new;

}



// recursive function that frees the subtree rooted at a given node 
void remove_dir(dir * root) {

  if (!root) {return;}
  
  int i;
  for (i = 0; i < root->subdirs; i++) {
    remove_dir(root->subdir[i]); 
  }

  free(root->subdir);
  free(root);

}


// function that generates a file full of random characters in the directory of the given node
void make_file(char * path, int label) {

  FILE * fp1 = NULL, * fp2 = NULL;

  char buff[1001];

  errno = 0;

  sprintf(buff, "/mnt/X%s/%d", path, label);
  fp1 = fopen(buff, "w");
  if (!fp1) {
    printf("\n*** FILE: %s failed to open ***\n", buff);
  }
  if (!copy) {
    sprintf(buff, "/mnt/Y%s/%d", path, label);
    fp2 = fopen(buff, "w");
    if (!fp2) {
     printf("\n*** FILE: %s failed to open ***\n", buff);
    }
  }

  if (errno != ENOSPC) {

    int size = rand()%(file_size_max + 1 - file_size_min) + file_size_min;

    int i,j;
    buff[1000] = '\0';
    for (i = 0; i < size; i++) {
      for (j = 0; j < 1000; j ++) { buff[j] = rand()%90 + 32; }
      fprintf(fp1, "%s", buff);
      if (!copy) { fprintf(fp2, "%s", buff); }
    }

    mem_count += size;
    used[label] = size;
    file_count++;

  } else {
    FULL = 1;
  }

  if (fp1) { fclose(fp1); }
  if (fp2) { fclose(fp2); }

}

// function that randomly adds a new file and subdirectires to the given directory
void make_random(dir * root, int label) {

  dir * DIR = root;
  char path[1000];
  sprintf(path, "%s", root->name);
  int index = 4;
  int action = 0;
  
  while (DIR) {
    if (dir_count < dir_lim && DIR->subdirs < branch_factor) {
      action = rand()%(DIR->subdirs + 2);
    } else {
      action = rand()%(DIR->subdirs + 1);
    }

    if (!action) {
      break;
    } else if (!DIR->subdir[action-1]) {
      DIR->subdir[action-1] = make_dir(path);
      DIR->subdirs++;
    }

    DIR = DIR->subdir[action-1];
    sprintf(path+index, "%s", DIR->name);
    index += 4;
  }

  make_file(path, label);
  DIR->files++;

}


// remove and repalce with added optional write test
void RandR(dir * root, int lim, int test) {

  char buff[1000];
  int * removed = malloc(lim*sizeof(int));
  int i, label = rand()%file_count, error = 0;
  for (i = 0; i < lim; i++) {
    while (!used[label]) { label = rand()%file_count; }
    sprintf(buff, "find /mnt/X -type f -name \"%d\" -exec rm -f {} \\;", label);
    error = system(buff);
    if (error) { fprintf(stderr, "\nERROR : %d  when trying to remove file %d \n", error, label); }
    if (!copy) {
      sprintf(buff, "find /mnt/Y -type f -name \"%d\" -exec rm -f {} \\;", label);
      error = system(buff);
      if (error) { fprintf(stderr, "\nERROR : %d  when trying to remove file %d \n", error, label); }
    }
    mem_count -= used[label];
    used[label] = 0;
    removed[i] = label;
    file_count--;
  }

  // write test, this is meant to measure write performance but is not currently working
  {
    int fd, ret;
    char * w_buff;
    struct timespec start, end;
    double time;

    if (test) {
      w_buff = malloc(100000000);
      for (i = 0; i < 100000000; i++) { w_buff[i] = (char)(rand()%90 + 32); }

      sync();

      system("du -sh /mnt/X\ndu -sh /mnt/Y");

      sprintf(buff, "umount %s &> /dev/null", partX);
      do { ret = system(buff); } while (ret);
      system("sync; echo 3 > /proc/sys/vm/drop_caches");
      sprintf(buff, "mount -t %s %s /mnt/X", file_sys, partX);
      system(buff);

      fd = open("/mnt/X/test", O_RDWR | O_CREAT, S_IWRITE | S_IREAD);

      clock_gettime(CLOCK_MONOTONIC, &start);
      ret = pwrite(fd, w_buff, 100000000);
      fsync(fd);
      close(fd);
      clock_gettime(CLOCK_MONOTONIC, &end);

      system("rm /mnt/X/test");

      time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0;

      if (ret < 0)
        fprintf(stderr, "\n***** ERROR: write failed -- %s *****\n", strerror(errno));
      else if (ret != 100000000)
        fprintf(stderr, "*** short write: %d bytes ***\n", ret);

      fprintf(stderr, "Write for X:\t %lf MB/sec\n", 100/time);

      sprintf(buff, "umount %s &> /dev/null", partY);
      do { ret = system(buff); } while (ret);
      system("sync; echo 3 > /proc/sys/vm/drop_caches");
      sprintf(buff, "mount -t %s %s /mnt/Y", file_sys, partY);
      system(buff);

      fd = open("/mnt/Y/test", O_RDWR | O_CREAT, S_IWRITE | S_IREAD);

      clock_gettime(CLOCK_MONOTONIC, &start);
      ret = write(fd, w_buff, 100000000);
      fsync(fd);
      close(fd);
      clock_gettime(CLOCK_MONOTONIC, &end);

      system("rm /mnt/Y/test");

      time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0;

      if (ret < 0)
        fprintf(stderr, "\n***** ERROR: write failed -- %s *****\n", strerror(errno));
      else if (ret != 100000000)
        fprintf(stderr, "*** short write: %d bytes ***\n", ret);

      fprintf(stderr, "Write for Y:\t %lf MB/sec\n", 100/time);

      free(w_buff);
    }

  }

  FULL = 0;

  dir * DIR = root;
  char path[1000];
  sprintf(path, "%s", root->name);
  int index = 4;
  int action = 0;

  while (DIR) {
    if (DIR->subdirs < branch_factor) {
      action = rand()%(DIR->subdirs + 1);
    } else {
      action = rand()%(DIR->subdirs);
    }

    if (DIR->subdir[action]) {
      DIR = DIR->subdir[action];
      sprintf(path+index, "%s", DIR->name);
      index += 4;
    } else {
      DIR->subdir[action] = make_dir(path);
      DIR->subdirs++;
      sprintf(path+index, "%s", DIR->subdir[action]->name);
      break;
    }
  }

  for (i = 0; i < lim; i++) {
    make_file(path, removed[i]);
    if (FULL) { break; }
  }

  free(removed);
}

// function that performs timed greps on both directories
void grep_test() {

  int fail;
  struct timespec start, end;
  double time;
  char buff[100];

  fprintf(stderr, "\nperforming grep on partition X:");
  sprintf(buff, "umount %s", partX);
  system(buff);
  system("sync; echo 3 > /proc/sys/vm/drop_caches");
  sprintf(buff, "mount -t %s %s /mnt/X", file_sys, partX);
  system(buff);

  clock_gettime(CLOCK_MONOTONIC, &start);
  system("grep -r \" \" /mnt/X/AAA > /dev/null");
  clock_gettime(CLOCK_MONOTONIC, &end);
  time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0;

  fprintf(stderr, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);

  if (copy) {
    fprintf(stderr, "Reformatting Y and copying over contents of X ... \n");
    sprintf(buff, "umount %s", partY);
    system(buff);
    sprintf(buff, "mkfs.ext4 -q %s -L Y", partY);
    system(buff);
    sprintf(buff, "mount -t ext4 %s /mnt/Y", partY);
    fail = system(buff);
    if (fail) {
      fprintf(stderr, "**** Failed to Mount %s ****", partY);
      return;
    }
    system("cp -a /mnt/X/AAA /mnt/Y/AAA");
  }

  fprintf(stderr, "performing grep on partition Y:");
  sprintf(buff, "umount %s", partY);
  system(buff);
  system("sync; echo 3 > /proc/sys/vm/drop_caches");
  sprintf(buff, "mount -t %s %s /mnt/Y", file_sys, partY);
  system(buff);

  clock_gettime(CLOCK_MONOTONIC, &start);
  system("grep -r \" \" /mnt/Y/AAA > /dev/null");
  clock_gettime(CLOCK_MONOTONIC, &end);
  time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0;

  fprintf(stderr, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);

  printf("\n");
}


// some settings are not modular yet

int main(int argc, char ** argv) {

  // default settings
  branch_factor = 10;
  dir_lim = 1000;
  mem_lim = 5000000;
  file_size_min = 1;
  file_size_max = 150;
  file_sys = "ext4";
  partX = "/dev/sda5";
  partY = "/dev/sda6";

  int i;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-c")) { copy = 1; }
    if (!strcmp(argv[i], "-w")) { write_test = 1; }
    if (!strcmp(argv[i], "-x")) {
      partX = malloc(strlen(argv[++i])+1);
      strcpy(partX, argv[i]);
    }
    if (!strcmp(argv[i], "-y")) {
      partY = malloc(strlen(argv[++i])+1);
      strcpy(partY, argv[i]);
    }
    if (!strcmp(argv[i], "-l")) { mem_lim = atoi(argv[++i]); }
    if (!strcmp(argv[i], "-btrfs")) { file_sys = "btrfs"; }
  }

  int fail = 0;
  char buff[1000];

  fprintf(stderr, "\n**** FILL TEST ****\n\nSettings:\nBranch Factor = %d\nDir Limit = %d\nFile Size Min = %d\nFile Size Max = %d\nFile System = %s\n\n", branch_factor, dir_lim, file_size_min, file_size_max, file_sys);

  sprintf(buff, "hdparm -t %s", partX);
  system(buff);
  system(buff);
  system(buff);

  sprintf(buff, "hdparm -t %s", partY);
  system(buff);
  system(buff);
  system(buff);

  sprintf(buff, "umount %s", partX);
  system(buff);
  system("mkdir /mnt/X &> /dev/null");
  if (!strcmp(file_sys, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s -L X", partX);
  else
    sprintf(buff, "mkfs.ext4 -q %s -L X", partX);
  system(buff);
  sprintf(buff, "mount -t %s %s /mnt/X", file_sys, partX);
  fail = system(buff);

  if (fail) {
    fprintf(stderr, "**** Failed to Mount dev/sda5 ****");
    return 1;
  }

  sprintf(buff, "umount %s", partY);
  system(buff);
  system("mkdir /mnt/Y &> /dev/null");
  if (!strcmp(file_sys, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s -L Y", partY);
  else
    sprintf(buff, "mkfs.ext4 -q %s -L Y", partY);
  system(buff);
  sprintf(buff, "mount -t %s %s /mnt/Y", file_sys, partY);
  fail = system(buff);

  if (fail) {
    fprintf(stderr, "**** Failed to Mount /dev/sda6 ****");
    return 1;
  }

  dir * root = make_dir("");

  int test_num = 1;

  fprintf(stderr, "\ncreating random directories and files ... \n");

  while (!FULL && mem_count < mem_lim) { make_random(root, file_count); }

  fprintf(stderr, "kB written:\t%d\t\tfiles created:\t%d\n", mem_count, file_count);
  
  grep_test();

  for (i = 0; i < 800; i++) {
    if ((i+1)%100 == 0) {
      fprintf(stderr, "\nRemove and Refill round %d\nCurrent Volume: %d kB,  File Count: %d\n", ++i, mem_count, file_count);
      RandR(root, file_count/20, write_test);
      grep_test();
    }
    RandR(root, file_count/20, 0);
  }

  remove_dir(root);

  return 0;

}
