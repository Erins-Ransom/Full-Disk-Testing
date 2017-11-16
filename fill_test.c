
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
#include <sys/mount.h>
#include <fcntl.h>


// counters and settings
static int branch_factor;
static int dir_count;
static int mem_count;
static int mem_lim;
static int file_count;
static int big_file_count;
static int dir_lim;
static int big_file_size_max;
static int file_size_max;
static int file_size_min;
static int RandR_index;
static int used[100000];
static double t1, t2;
static short int FULL;
static short int copy;
static short int write_test;
static short int block_trace;
static short int big_files;
static char * file_sys;
static char * partX;
static char * partY;
static char * partZ;



// tree struct to keep track of the directories being made
typedef struct dir {

  char name[5];
  struct dir ** subdir;
  int subdirs;
  int files;

} dir;


// function to genertate a new node in the directory tree and in the file systems
dir * make_dir(char * path);

// recursive function that frees the subtree rooted at a given node 
void remove_dir(dir * root);

// function that generates a file full of random characters in the directory of the given node
void make_file(char * path, int label, int big_file);

// function that randomly adds a new file and subdirectires to the given directory
void make_random(dir * root, int label);

// remove and repalce with added optional write test
void RandR(dir * root, int lim, int test);

// function that performs timed greps on both directories
void grep_test();

void layout_test(char * part, char * directory);



// some settings are not modular yet

int main(int argc, char ** argv) {

  // default settings
  branch_factor = 10;
  dir_lim = 1000;
  mem_lim = 4750000;
  file_size_min = 1;
  file_size_max = 150;
  big_file_size_max = 200000;
  file_sys = "ext4";
  partX = "/dev/sda5";
  partY = "/dev/sda6";
  partZ = "/dev/sda7";

  int fail = 0;
  int i;
  char buff[1000];
  FILE * out = NULL;

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
    if (!strcmp(argv[i], "-bt")) { block_trace = 1; }
    if (!strcmp(argv[i], "-bf")) { big_files = 1; }
  }


//  sprintf(buff, "fill_test_%s.out", file_sys);
//  FILE * out = fopen(buff, "w");
//  if (!out) { fprintf(stdout, "Failled to open output file\n"); return 1; }
//  fclose(out);
  sprintf(buff, "%s_X.out", file_sys);
  out = fopen(buff, "w");
  fclose(out);
  sprintf(buff, "%s_Y.out", file_sys);
  out = fopen(buff, "w");
  fclose(out);
  sprintf(buff, "%s_Z.out", file_sys);
  out = fopen(buff, "w");
  fclose(out);
  sprintf(buff, "%s_X_del.out", file_sys);
  out = fopen(buff, "w");
  fclose(out);
  sprintf(buff, "%s_X_write.out", file_sys);
  out = fopen(buff, "w");
  fclose(out);
  sprintf(buff, "%s_Y_del.out", file_sys);
  out = fopen(buff, "w");
  fclose(out);
  sprintf(buff, "%s_Y_write.out", file_sys);
  out = fopen(buff, "w");
  fclose(out);


//  fprintf(out, "\n**** FILL TEST ****\n\nSettings:\nBranch Factor = %d\nDir Limit = %d\nFile Size Min = %d kB\nFile Size Max = %d kB\nBig File Size Max = %d kB\nFile System = %s\nPartition X = %s\nPartition Y = %s\nPartition Z = %s\n\n", branch_factor, dir_lim, file_size_min, file_size_max, big_file_size_max, file_sys, partX, partY, partZ);
  fprintf(stdout, "\n**** FILL TEST ****\n\nSettings:\nBranch Factor = %d\nDir Limit = %d\nFile Size Min = %d kB\nFile Size Max = %d kB\nBig File Size Max = %d kB\nFile System = %s\nPartition X = %s\nPartition Y = %s\nPartition Z = %s\n\n", branch_factor, dir_lim, file_size_min, file_size_max, big_file_size_max, file_sys, partX, partY, partZ);

//  fflush(out);
  sync();

  sprintf(buff, "umount %s", partX);
  fail = system(buff);
  if (fail) { fprintf(stderr, "**** Failed to Unmount %s ****\n", partX); }

  if (!strcmp(file_sys, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s -L X", partX);
  else
    sprintf(buff, "mkfs.ext4 -q %s -L X", partX);
  system(buff);

  sprintf(buff, "umount %s", partY);
  fail = system(buff);
  if (fail) { fprintf(stderr, "**** Failed to Unmount %s ****\n", partY); }

  if (!strcmp(file_sys, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s -L Y", partY);
  else
    sprintf(buff, "mkfs.ext4 -q %s -L Y", partY);
  system(buff);

  sprintf(buff, "umount %s", partZ);
  fail = system(buff);
  if (fail) { fprintf(stderr, "**** Failed to Unmount %s ****\n", partZ); }

  if (!strcmp(file_sys, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s -L Z", partZ);
  else
    sprintf(buff, "mkfs.ext4 -q %s -L Z", partZ);
  system(buff);



  sprintf(buff, "hdparm -t %s", partY);
  system(buff);
  system(buff);
  system(buff);

  sprintf(buff, "hdparm -t %s", partX);
  system(buff);
  system(buff);
  system(buff);

  sprintf(buff, "hdparm -t %s", partZ);
  system(buff);
  system(buff);
  system(buff);

  system("mkdir /mnt/X");
  sprintf(buff, "mount -t %s %s /mnt/X", file_sys, partX);
  fail = system(buff);

  if (fail) { fprintf(stderr, "**** Failed to Mount %s ****\n", partX); return 1; } 

  system("mkdir /mnt/Y");
  sprintf(buff, "mount -t %s %s /mnt/Y", file_sys, partY);
  fail = system(buff);

  if (fail) { fprintf(stderr, "**** Failed to Mount %s ****\n", partY); return 1; }

  system("mkdir /mnt/Z");
  sprintf(buff, "mount -t %s %s /mnt/Z", file_sys, partZ);
  fail = system(buff);

  if (fail) { fprintf(stderr, "**** Faild to Mount %s ****\n", partY); return 1; }

  dir * root = make_dir("");

  int test_num = 1;

//  fprintf(out, "\ncreating random directories and files ... \n");
  fprintf(stdout, "\ncreating random directories and files ... \n");

  while (!FULL) { make_random(root, file_count); }

//  fprintf(out, "kB written:\t%d\t\tfiles created:\t%d (%d big files)\n", mem_count, file_count, big_file_count);
  fprintf(stdout, "kB written:\t%d\t\tfiles created:\t%d (%d big files)\n", mem_count, file_count, big_file_count);

//  fprintf(out, "Average Write Time (X, Y): \t%f, %f\n", write_time[0][0]/mem_count, write_time[1][0]/mem_count);
  fprintf(stdout, "Average Write Time (X, Y): \t%f, %f\n", t1/mem_count, t2/mem_count);

  grep_test();
  if (block_trace) {
    layout_test(partX, "/mnt/X/AAA");
    layout_test(partY, "/mnt/Y/AAA");
    layout_test(partZ, "/mnt/Z/AAA");
  }

  fflush(out);

  fprintf(out, "\n");
  fprintf(stdout, "\n");
  for (i = 0; i < 600; i++) {
//    fprintf(stderr, "|");
//    fflush(stderr);
    if ((i+1)%50 == 0) {
//      fprintf(out, "\nRemove and Refill round %d\nCurrent Volume: %d kB,  File Count: %d (%d big files)\n", ++i, mem_count, file_count, big_file_count);
      fprintf(stdout, "\nRemove and Refill round %d\nCurrent Volume: %d kB,  File Count: %d (%d big files)\n", ++i, mem_count, file_count, big_file_count);
      RandR(root, 250000, write_test);
      grep_test();
      if (block_trace) {
        layout_test(partX, "/mnt/X/AAA");
        layout_test(partY, "/mnt/Y/AAA");
      }
    }
    RandR(root, 250000, 0);
    RandR_index++;
//    fflush(out);
  }

  remove_dir(root);

//  fprintf(out, "\nAverage Delete Times (X, Y):\n");
//  for (i=0; i < 600; i++); { fprintf(out, "%f, %f\n", delete_time[0][i], delete_time[1][i]); }

//  fprintf(out, "\nAverage Write Times (X, Y):\n");
//  for (i=0; i < 600; i++); { fprintf(out, "%f, %f\n", write_time[0][i], write_time[1][i]); }

//  fclose(out);

  return 0;

}


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


void remove_dir(dir * root) {

  if (!root) {return;}

  int i;
  for (i = 0; i < root->subdirs; i++) {
    remove_dir(root->subdir[i]);
  }

  free(root->subdir);
  free(root);

}


void make_file(char * path, int label, int big_file) {

  FILE * fp1 = NULL, * fp2 = NULL;

  char buff[1000];
  char * contents;

  struct timespec start, end;

  errno = 0;

  sprintf(buff, "/mnt/X%s/%d", path, label);
  fp1 = fopen(buff, "w");
  if (!fp1) {
    fprintf(stderr,  "\n*** FILE: %s failed to open ***\n", buff);
  }

  sprintf(buff, "/mnt/Y%s/%d", path, label);
  fp2 = fopen(buff, "w");
  if (!fp2) {
    fprintf(stderr, "\n*** FILE: %s failed to open ***\n", buff);
  }


  if (errno != ENOSPC) {
    int size;

    if (!big_file) {
      size = rand()%(file_size_max + 1 - file_size_min) + file_size_min;
    } else {
      size = rand()%(big_file_size_max + 1 - file_size_max) + file_size_max;
      big_file_count++;
    }

    if (size + mem_count > mem_lim) { size = mem_lim - mem_count; }

    int i;
    contents = malloc(size*1000 + 1);
    contents[size*1000] = '\0';
    for (i = 0; i < size*1000; i++) { contents[i] = rand()%90 + 32; }

    fsync();
    clock_gettime(CLOCK_MONOTONIC, &start);
    fprintf(fp1, "%s", contents);
    fflush(fp1);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &end);
    t1 += 1000*((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0);

    fsync();
    clock_gettime(CLOCK_MONOTONIC, &start);
    fprintf(fp2, "%s", contents);
    fflush(fp2);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &end);
    t2 += 1000*((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0);

    mem_count += size;
    used[label] = size;
    file_count++;

  } else {
    FULL = 1;
  }

  free(contents);
  if (mem_count >= mem_lim) { FULL = 1; }
  if (fp1) { fclose(fp1); }
  if (fp2) { fclose(fp2); }

}


void make_random(dir * root, int label) {

  dir * DIR = root;
  char path[1000];
  sprintf(path, "%s", root->name);
  int index = 4;
  int action = 0;
//  int big_file = 0;

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

//  if (big_files) {
//    big_file = !(rand()%10000);
//  }

  make_file(path, label, 0);
  DIR->files++;

}



void RandR(dir * root, int lim, int test) {

  char buff[1000];
  int i, label = rand()%file_count, error = 0, big_file = 0;
  struct timespec start, end;
  FILE * fp1, * fp2 = NULL;

  sprintf(buff, "%s_X_del,out", file_sys);
  fp1 = fopen(buff, "a");
  sprintf(buff, "%s_Y_del.out", file_sys);
  fp2 = fopen(buff, "a");

  t1, t2 = 0;

  while (mem_count > mem_lim - lim) {
    while (!used[label]) { label = rand()%file_count; }
    sprintf(buff, "find /mnt/X -type f -name \"%d\" -exec rm -f {} \\;", label);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &start);
    error = system(buff);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &end);
    t1 += 1000*((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0);
    if (error) { fprintf(stderr, "\nERROR : %d  when trying to remove file %d \n", error, label); }
    sprintf(buff, "find /mnt/Y -type f -name \"%d\" -exec rm -f {} \\;", label);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &start);
    error = system(buff);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &end);
    t2 += 1000*((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0);
    if (error) { fprintf(stderr, "\nERROR : %d  when trying to remove file %d \n", error, label); }
    mem_count -= used[label];
    if (used[label] > file_size_max) { big_file_count--; }
    used[label] = 0;
    file_count--;

    i++;
  }

  fprintf(fp1, "%f\n", lim/t1);
  fprintf(fp2, "%f\n", lim/t2);
  fclose(fp1);
  fclose(fp2);

//  fprintf(stdout, "\nRemove and Refill Round: \t%d", RandR_index);
//  fprintf(stdout, "\nDelete Time (X,Y):\t%f, %f", delete_time[0][RandR_index], delete_time[1][RandR_index]);

  FULL = 0;

  t1, t2 = 0;
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

  i = 0;
  while (!FULL) {
    if (big_files) { big_file = !(rand()%10000); }
    while (used[i]) { i++; }
    make_file(path, i, big_file);
  }

  sprintf(buff, "%s_X_write.out", file_sys);
  fp1 = fopen(buff, "a");
  sprintf(buff, "%s_Y_write.out", file_sys);
  fp2 = fopen(buff, "a");

  fprintf(fp1, "%f\n", lim/t1);
  fprintf(fp2, "%f\n", lim/t2);

  fclose(fp1);
  fclose(fp2);

}



void grep_test() {

  int fail;
  struct timespec start, end;
  double time;
  char buff[100];
  FILE * fp = NULL;

//  fprintf(out, "\nperforming grep on partition X:");
  fprintf(stdout, "\nperforming grep on partition X:");

  sprintf(buff, "umount %s", partX);
  system(buff);
  system("sync; echo 3 > /proc/sys/vm/drop_caches");
  sprintf(buff, "mount -t %s %s /mnt/X", file_sys, partX);
  system(buff);

  clock_gettime(CLOCK_MONOTONIC, &start);
  system("grep -r \" \" /mnt/X/AAA > /dev/null");
  clock_gettime(CLOCK_MONOTONIC, &end);
  time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0;

  sprintf(buff, "%s_X.out", file_sys);
  fp = fopen(buff, "a");
//  fprintf(out, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);
  fprintf(fp, "%f\n", (mem_count/1000)/time);
  fclose(fp);
  fprintf(stdout, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);

//  fprintf(out, "performing grep on partition Y:");
  fprintf(stdout, "performing grep on partition Y:");

  sprintf(buff, "umount %s", partY);
  system(buff);
  system("sync; echo 3 > /proc/sys/vm/drop_caches");
  sprintf(buff, "mount -t %s %s /mnt/Y", file_sys, partY);
  system(buff);

  clock_gettime(CLOCK_MONOTONIC, &start);
  system("grep -r \" \" /mnt/Y/AAA > /dev/null");
  clock_gettime(CLOCK_MONOTONIC, &end);
  time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0;

  sprintf(buff, "%s_Y.out", file_sys);
  fp = fopen(buff, "a");
//  fprintf(out, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);
  fprintf(fp, "%f\n", (mem_count/1000)/time);
  fclose(fp);
  fprintf(stdout, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);

  printf("\n");


  sync();
  fail = umount("/mnt/Z");
  if (fail)
    fprintf(stderr, "Failed to unmount: %s\r", strerror(errno));
  sync();
  if (!strcmp(file_sys, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s -L Z &> /dev/null", partZ);
  else
    sprintf(buff, "mkfs.ext4 -q %s -L Z", partZ);
  fail = system(buff);
  sprintf(buff, "mount -t %s %s /mnt/Z", file_sys, partZ);
  fail = system(buff);
  if (fail) {
    fprintf(stderr, "**** Failed to Mount %s ****\n", partZ);
    return;
  }

  system("cp -a /mnt/X/AAA /mnt/Z/.");

//  fprintf(out, "performing grep on partition Z:");
  fprintf(stdout, "performing grep on partition Z:");

  sprintf(buff, "umount %s", partZ);
  system(buff);
  system("sync; echo 3 > /proc/sys/vm/drop_caches");
  sprintf(buff, "mount -t %s %s /mnt/Z", file_sys, partZ);
  system(buff);

  clock_gettime(CLOCK_MONOTONIC, &start);
  system("grep -r \" \" /mnt/Z/AAA > /dev/null");
  clock_gettime(CLOCK_MONOTONIC, &end);
  time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0;

  sprintf(buff, "%s_Z.out", file_sys);
  fp = fopen(buff, "a");
//  fprintf(out, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);
  fprintf(fp, "%f\n", (mem_count/1000)/time);
  fclose(fp);
  fprintf(stdout, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);

  printf("\n");


}


void layout_test(char * part, char * directory) {
  char buff[100];

  fprintf(stderr, "\n Determining layout on %s... ", part);

  sync();
  sprintf(buff, "blktrace -a read -d %s -o tracefile &", part);
  system(buff);
  sprintf(buff, "grep -r \" \" %s > /dev/null", directory);
  system(buff);
  system("kill $(pidof -s blktrace)");
  sprintf(buff, "blkparse -a issue -f \"%%S %%n\\\\n\" -i tracefile | ./blk_interpreter");
  system(buff);

}



