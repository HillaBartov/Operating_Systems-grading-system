//Hilla Bartov 315636779
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>

int compareFiles(char file1[150], char file2[150], long size1, long size2);

int main(int argc, char *argv[]) {
    struct stat stat_p1, stat_p2;
    char *file1 = argv[1];
    char *file2 = argv[2];

    stat(file1, &stat_p1);
    stat(file2, &stat_p2);
    int compare;
    if (stat_p1.st_size <= stat_p2.st_size) {
        compare = compareFiles(file1, file2, stat_p1.st_size, stat_p2.st_size);
    } else {
        compare = compareFiles(file2, file1, stat_p2.st_size, stat_p1.st_size);
    }
    return compare;
}

/**
 *
 * @param file1 is for the smaller file
 * @param file2 is for the bigger file
 * @param size1 is the smaller file's size
 * @param size2 is the bigger file's size
 * @return 1-when identical, 2-when different, 3-when similar and -1 for error
 */
int compareFiles(char file1[150], char file2[150], long size1, long size2) {
    int fd1, fd2;
    char buff1[size1], buff2[size1];
    if ((fd1 = open(file1, O_RDONLY)) == -1 || (fd2 = open(file2, O_RDONLY)) == -1) {
        return -1;
    }
    if (read(fd1, buff1, size1) != size1 || read(fd2, buff2, size2) != size2) {
        return -1;
    }
    //when identical
    if (strcmp(buff1, buff2) == 0) {
        return 1;
    }
    long offsets = (size2 - size1) + 1, half;
    if (size1 % 2 == 0) {
        half = size1 / 2;
    } else {
        half = (size1 / 2) + 1;
    }
    long i, j, countSimilar;
    //check different offsets
    for (i = 0; i <= offsets; i++) {
        //get the offset in the bigger file
        if (i != 0 && lseek(fd2, i, SEEK_SET) == -1) {
            return -1;
        }
        //when the bigger file has only 1 character left for the offset
        if (i == offsets) {
            if (read(fd2, buff2, 1) != 1) {
                return -1;
            }
            //ignore rotation one- already read once above,and later read by the size of the smaller file for the check*
        } else if (i != 0 && read(fd2, buff2, size1) != size1) {
            return -1;
        }
        //compare the files in current offset
        countSimilar = 0;
        //*check matches by the smaller file size
        for (j = 0; j < size1; ++j) {
            if (buff1[j] == buff2[j]) {
                countSimilar++;
            }
            if (countSimilar == half) {
                //When at least half of the characters are similar
                return 3;
            }
        }
    }
    close(fd1);
    close(fd2);
    return 2;
}
