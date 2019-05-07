
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
static int branch_factor;   // hom hand subdirectories we allow in a given directory
static int dir_count;
static int mem_count;       // totla size of files wrtien in kB
static int mem_lim;
static int file_count;
static int dir_lim;
static int file_size_max;
static int file_size_min;
static int rounds;          // how many remove and replace rounds to run
static int used[100000];    // file labels used/size, file i has size used[i] or used[i]==0 if i is not a file name in use
static double t1, t2;       // use these to time writes
static short int FULL;
static short int copy;
static short int block_trace;
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

// function that generates a file named [label] full of random characters in the directory of the given node
void make_file(char * path, int label);

// function that randomly adds a new file and subdirectires to the given directory
// it descendes the tree at random, possibly stopping and creating a file and possibly creating
// new directories to descend into
void make_random(dir * root, int label);

// remove 5% of files at random and replace with new files in single new directory
void RandR(dir * root, int lim, int call_lim);

// function that performs timed greps on both partitions
void grep_test(int call_lim);

// get the layout score of both partitions 
void layout_test(char * part, char * directory);





int main(int argc, char ** argv) {

  // default settings
  branch_factor = 10;
  dir_lim = 1000;
  mem_lim = 4750000;
  file_size_min = 1;
  file_size_max = 150;
  rounds = 600;
  file_sys = "ext4";
  partX = "/dev/sdc1";
  partY = "/dev/sdc2";
  partZ = "/dev/sdc3";

  int fail = 0;
  int i;
  char buff[1000];

  // parse settings (not all are modular yet)
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-c")) 
      copy = 1;
    if (!strcmp(argv[i], "-x"))
      partX = argv[++i];
    if (!strcmp(argv[i], "-y"))
      partY = argv[++i];
    if (!strcmp(argv[i], "-l")) 
      mem_lim = atoi(argv[++i]);
    if (!strcmp(argv[i], "-fs")) 
      file_sys = argv[++i];
    if (!strcmp(argv[i], "-bt"))
      block_trace = 1;
  }


  fprintf(stdout, "\n**** FILL TEST ****\n\nSettings:\nBranch Factor = %d\nDir Limit = %d\nFile Size Min = %d kB\nFile Size Max = %d kB\nFile System = %s\nRounds = %d\nPartition X = %s\nPartition Y = %s\nPartition Z = %s\n\n", branch_factor, dir_lim, file_size_min, file_size_max, file_sys, rounds, partX, partY, partZ);

  sync();

  sprintf(buff, "umount %s", partX);
  fail = system(buff);
  if (fail) { fprintf(stderr, "**** Failed to Unmount %s ****\n", partX); }

  if (!strcmp(file_sys, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s", partX);
  else if (!strcmp(file_sys, "xfs"))
    sprintf(buff, "mkfs.xfs -q -f %s", partX);
  else if (!strcmp(file_sys, "f2fs"))
    sprintf(buff, "mkfs.f2fs %s", partX);
  else
    sprintf(buff, "mkfs.ext4 -q %s", partX);
  system(buff);

  sprintf(buff, "umount %s", partY);
  fail = system(buff);
  if (fail) { fprintf(stderr, "**** Failed to Unmount %s ****\n", partY); }

  if (!strcmp(file_sys, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s", partY);
  else if (!strcmp(file_sys, "xfs"))
    sprintf(buff, "mkfs.xfs -q -f %s", partY);
  else if (!strcmp(file_sys, "f2fs"))
    sprintf(buff, "mkfs.f2fs %s", partY);
  else
    sprintf(buff, "mkfs.ext4 -q %s", partY);
  system(buff);

  sprintf(buff, "umount %s", partZ);
  fail = system(buff);
  if (fail) { fprintf(stderr, "**** Failed to Unmount %s ****\n", partZ); }

  if (!strcmp(file_sys, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s", partZ);
  else if (!strcmp(file_sys, "xfs"))
    sprintf(buff, "mkfs.xfs -q -f %s", partZ);
  else if (!strcmp(file_sys, "f2fs"))
    sprintf(buff, "mkfs.f2fs %s", partZ);
  else
    sprintf(buff, "mkfs.ext4 -q %s", partZ);
  system(buff);



  sprintf(buff, "hdparm -t %s", partX);
  system(buff);
  system(buff);
  system(buff);

  sprintf(buff, "hdparm -t %s", partY);
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

  fprintf(stdout, "\ncreating random directories and files ... \n");

  while (!FULL) { make_random(root, file_count); }

  fprintf(stdout, "kB written:\t%d\t\tfiles created:\t%d\n", mem_count, file_count);
  fprintf(stdout, "Average Write Time (part X, part Y): \t%f MB/sec, %f MB/sec\n", mem_count/t1, mem_count/t2);

  grep_test(rounds/50);
  if (block_trace) {
    layout_test(partX, "/mnt/X/AAA");
    layout_test(partY, "/mnt/Y/AAA");
    layout_test(partZ, "/mnt/Z/AAA");
  }


  fprintf(stdout, "\n");
  for (i = 0; i < rounds; i++) {
    if ((i+1)%50 == 0) {
      fprintf(stdout, "\nRemove and Refill round %d\nCurrent Volume: %d kB,  File Count: %d\n", i+1, mem_count, file_count);
      RandR(root, mem_lim/20, rounds);
      grep_test(rounds/50);
      if (block_trace) {
        layout_test(partX, "/mnt/X/AAA");
        layout_test(partY, "/mnt/Y/AAA");
        layout_test(partZ, "/mnt/Z/AAA");
      }
    }
    RandR(root, mem_lim/20, rounds);
  }

  remove_dir(root);

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


void make_file(char * path, int label) {

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
    int size = rand()%(file_size_max + 1 - file_size_min) + file_size_min;

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
    
    free(contents);

  } else {
    FULL = 1;
  }

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



void RandR(dir * root, int lim, int call_lim) {

  char buff[1000];
  int i, label = rand()%file_count, error = 0;
  struct timespec start, end;
  static  FILE * fpXd, * fpXw, * fpYd, *fpYw;
  static int calls;

  if (!fpXd) {
    sprintf(buff, "%s_X_del.out", file_sys);
    fpXd = fopen(buff, "w");
  }
  if (!fpYd) {
    sprintf(buff, "%s_Y_del.out", file_sys);
    fpYd = fopen(buff, "w");
  }
  if (!fpXw) {
    sprintf(buff, "%s_X_write.out", file_sys);
    fpXw = fopen(buff, "w");
  }
  if (!fpYw) {
    sprintf(buff, "%s_Y_write.out", file_sys);
    fpYw = fopen(buff, "w");
  }

  if (!(fpYw && fpXw && fpYd && fpXd)) { fprintf(stderr, "**** Failed to open output file **** \n"); }

  calls++;
  t1 = 0;
  t2 = 0;

  // remove files at random until we pass under the given limit
  while (mem_count > mem_lim - lim) {

    // choose file to remove
    while (!used[label]) { label = rand()%file_count; }

    // find and remove from partition X
    sprintf(buff, "find /mnt/X -type f -name \"%d\" -exec rm -f {} \\;", label);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &start);
    error = system(buff);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &end);
    t1 += 1000*((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0);
    if (error) { fprintf(stderr, "\nERROR : %d  when trying to remove file %d \n", error, label); }

    // find and remove from partitioin Y
    sprintf(buff, "find /mnt/Y -type f -name \"%d\" -exec rm -f {} \\;", label);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &start);
    error = system(buff);
    fsync();
    clock_gettime(CLOCK_MONOTONIC, &end);
    t2 += 1000*((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0);
    if (error) { fprintf(stderr, "\nERROR : %d  when trying to remove file %d \n", error, label); }

    // book keeping
    mem_count -= used[label];
    used[label] = 0;
    file_count--;

    i++;
  }

  // we keep a cumulative time for the removal of the files, then average their total
  // size over the cululative time to remove them
  // *NOTE* times are multiplied by 1000 (^ done earlier ^) since we want MB/sec and [lim] is in kB 
  fprintf(fpXd, "%f\n", lim/t1);
  fprintf(fpYd, "%f\n", lim/t2);

  // reset counters
  FULL = 0;
  t1 = 0;
  t2 = 0;
  dir * DIR = root;
  char path[1000];
  sprintf(path, "%s", root->name);
  int index = 4;
  int action = 0;

  // choose place for replacement files
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

  // make new files
  i = 0;
  while (!FULL) {
    while (used[i]) { i++; }
    make_file(path, i);
  }

  // times are again cumulative and we average their total size over total time
  fprintf(fpXw, "%f\n", lim/t1);
  fprintf(fpYw, "%f\n", lim/t2);

  fflush(fpXd);
  fflush(fpXw);
  fflush(fpYd);
  fflush(fpYw);

  if (calls == call_lim) {
    fclose(fpXd);
    fclose(fpXw);
    fclose(fpYd);
    fclose(fpYw);
  }

}



void grep_test(int call_lim) {

  int fail;
  struct timespec start, end;
  double time;
  char buff[100];
  static FILE * fpX, * fpY, * fpZ;
  static int calls;

  if (!fpX) {
    sprintf(buff, "%s_X.out", file_sys);
    fpX = fopen(buff, "w");
  }
  if (!fpY) {
    sprintf(buff, "%s_Y.out", file_sys);
    fpY = fopen(buff, "w");
  }
  if (!fpZ && copy) {
    sprintf(buff, "%s_Z.out", file_sys);
    fpZ = fopen(buff, "w");
  }
  if (!(fpX && fpY && (fpZ || !copy))) { fprintf(stderr, "**** Failed to open output file **** \n"); }

  calls++;

  // for both partitionis we perform a recursive grep over the whole partition then take size over time

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

  fprintf(fpX, "%f\n", (mem_count/1000)/time);
  fprintf(stdout, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);

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

  fprintf(fpY, "%f\n", (mem_count/1000)/time);
  fprintf(stdout, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);

  // if we are want to test a clean copy, we copy over the contents to a freshly formatted partition
  if (copy) {
    sync();
    sprintf(buff, "umount %s", partZ);
    sysetm(buff);
    sync();
    if (!strcmp(file_sys, "btrfs"))
      sprintf(buff, "mkfs.btrfs -f %s &> /dev/null", partZ);
    else if (!strcmp(file_sys, "xfs"))
      sprintf(buff, "mkfs.xfs -q -f %s", partZ);
    else if (!strcmp(file_sys, "f2fs"))
      sprintf(buff, "mkfs.f2fs %s", partZ);
    else
      sprintf(buff, "mkfs.ext4 -q %s", partZ);
    fail = system(buff);
    sprintf(buff, "mount -t %s %s /mnt/Z", file_sys, partZ);
    fail = system(buff);
    if (fail) {
      fprintf(stderr, "**** Failed to Mount %s ****\n", partZ);
      return;
    }

    system("cp -a /mnt/X/AAA /mnt/Z/.");

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

    fprintf(fpZ, "%f\n", (mem_count/1000)/time);
    fprintf(stdout, "\t %lf sec, ~%lf MB/sec\n", time, (mem_count/1000)/time);
    
    fflush(fpZ);
    if (calls == call_lim)
       fclose(fpZ);
  }

  printf("\n");

  fflush(fpX);
  fflush(fpY);

  if (calls == call_lim) {
    fclose(fpX);
    fclose(fpY);
  }

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


/*
static int setup_dev(char * dev, char * mnt) {

  char buff[100];
  int fail = 0;

  if (strcmp(file_sys, "betrfs")) {
    sprintf(buff, "sudo mkfs -t ext4 -E lazy_itable_init=0,lazy_journal_init=0 %s", dev);
    fail = system(buff);
    if (fail) { return fail; }

    system("sudo mkdir -p "MNT);
    fail = system("sudo mount -t ext4 "SB_DEV" "MNT);
    if (fail) { return fail; }

    system("sudo rm -rf "MNT"/*\nsudo mkdir "MNT"/db\nsudo mkdir "MNT"/dev\nsudo touch "MNT"/dev/null\nsudo mkdir "MNT"/tmp\n");
    sync();
    system("sudo chmod 1777 "MNT"/tmp\n");
    fail = system("sudo umount "MNT);
    if (fail) { return fail; }

    system("sudo modprobe zlib");
    fail = system("sudo insmod "REPO"/filesystem/ftfs.ko sb_dev="SB_DEV" sb_fstype=ext4");
    if (fail) { return fail; }

    system("sudo touch dummy.dev\nsudo losetup /dev/loop0 dummy.dev\n");
    fail = system("sudo mount -t ftfs /dev/loop0 "MNT" -o max=128");

    if (!fail) {
      fprintf(stdout, "FTFS has been mounted, ");
    }
  }

  return fail;
}
*/
