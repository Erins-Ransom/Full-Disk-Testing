#include <stdio.h>
#include <sys/mount.h>
#include <string.h>

static char * file_system;
static char * partX;
static char * partY;


int initialize() {

  char buff[1000];
  int fail;

  system("mkdir /mnt/X");
  system("mkdir /mnt/Y");

  sync();

  sprintf(buff, "umount %s", partX);
  system(buff);
  sprintf(buff, "umount %s", partY);
  system(buff);

  if (!strcmp(file_system, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s -L X", partX);
  else
    sprintf(buff, "mkfs.ext4 -q %s -L X", partX);
  system(buff);

  sync();

  if (!strcmp(file_system, "btrfs"))
    sprintf(buff, "mkfs.btrfs -f %s -L Y", partY);
  else
    sprintf(buff, "mkfs.ext4 -q %s -L Y", partY);
  system(buff);

  sync();

  sprintf(buff, "mount -t %s %s /mnt/X", file_system, partX);
  fail = system(buff);
  if (fail) { fprintf(stderr, "**** FAILED TO MOUNT: %s ****\n", partX); return fail; }

  sprintf(buff, "mount -t %s %s /mnt/Y", file_system, partY);
  fail = system(buff);
  if (fail) { fprintf(stderr, "**** FAILED TO MOUNT: %s ****\n", partY); return fail; }

  system("cd /mnt/X\ngit init\ngit remote add origin https://github.com/torvalds/linux\ngit pull master\n");
  system("cd /mnt/Y\ngit init\ngit remote add origin https://github.com/torvalds/linux\ngit pull master\n");

  return 0;
}



int main(int argc, char ** argv) {

  file_system = "ext4";
  partX = "/dev/sda5";
  partY = "/dev/sda6";

  int ret;

  ret = initialize();

  if (ret) { return ret; }

  system("cd /mnt/X\ngit pull\n");
  system("cd /mnt/Y\ngit pull\n");

  return 0;
}
