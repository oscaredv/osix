#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
  int ret = EXIT_SUCCESS;
  if (argc == 1) {
    fprintf(stderr, "No file\n");
    ret = EXIT_FAILURE;
  }

  for (int i = 1; i < argc; i++) {
    int fd = open(argv[i], O_RDONLY);
    if (fd == -1) {
      fprintf(stderr, "%s: %s: No such file or directory\n", argv[0], argv[i]);
      ret = EXIT_FAILURE;
    } else {
      struct stat st;
      if (fstat(fd, &st) == -1) {
        fprintf(stderr, "%s: Can't stat %s\n", argv[0], argv[i]);
        ret = EXIT_FAILURE;
      } else {
        printf("Device=%d:%d inode no=%d\n", major(st.st_dev), minor(st.st_dev), st.st_ino);
        printf("Mode %o\n", st.st_mode);

        printf("nlinks=%d uid=%d gid=%d\n", st.st_nlink, st.st_uid, st.st_gid);
        printf("size=%d time=%d %s\n", st.st_size, st.st_time, ctime(&st.st_time));
        printf("direct: %d %d %d %d %d %d %d\n", st.st_direct[0], st.st_direct[1], st.st_direct[2], st.st_direct[3],
               st.st_direct[4], st.st_direct[5], st.st_direct[6]);
        printf("indirect %d dbl indirect=%d\n", st.st_indirect, st.st_dblindirect);
      }
      close(fd);
    }
  }

  return ret;
}
