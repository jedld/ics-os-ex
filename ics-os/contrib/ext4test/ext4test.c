/*
 * ext4 driver test: read a host-seeded file on /work (ext4 on virtio-blk),
 * then create + write + read back a new file, plus a subdirectory create.
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define SEED_PATH  "/work/seed.txt"
#define SEED_LEN   15
static const char seed[SEED_LEN] = "icsos-ext4-seed";

static int fail(const char *tag, const char *msg)
{
    printf("ext4test: FAIL %s errno=%d\n", msg, errno);
    printf("%s\n", tag);
    return 1;
}

int main(void)
{
    int fd, n;
    char buf[64];

    printf("ext4test: read seeded file\n");
    fd = open(SEED_PATH, O_RDONLY);
    if (fd < 0)
        return fail("EXT4_READ_FAIL", "open seed");
    memset(buf, 0, sizeof(buf));
    n = read(fd, buf, SEED_LEN);
    close(fd);
    if (n != SEED_LEN || memcmp(buf, seed, SEED_LEN) != 0)
        return fail("EXT4_READ_FAIL", "readback");
    printf("EXT4_READ_PASS\n");

    printf("ext4test: create + write + read back\n");
    fd = open("/work/ext4test.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return fail("EXT4_WRITE_FAIL", "create");
    n = write(fd, seed, SEED_LEN);
    if (n != SEED_LEN)
        return fail("EXT4_WRITE_FAIL", "write");
    if (fsync(fd) != 0)
        return fail("EXT4_WRITE_FAIL", "fsync");
    if (lseek(fd, 0, SEEK_SET) != 0)
        return fail("EXT4_WRITE_FAIL", "lseek");
    memset(buf, 0, sizeof(buf));
    if (read(fd, buf, SEED_LEN) != SEED_LEN || memcmp(buf, seed, SEED_LEN) != 0)
        return fail("EXT4_WRITE_FAIL", "readback");
    close(fd);
    printf("EXT4_WRITE_PASS\n");

    printf("ext4test: create directory + file in it\n");
    n = mkdir("/work/sub", 0777);
    if (n != 0)
        return fail("EXT4_DIR_FAIL", "mkdir");
    fd = open("/work/sub/inner.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return fail("EXT4_DIR_FAIL", "create in subdir");
    n = write(fd, seed, SEED_LEN);
    if (n != SEED_LEN)
        return fail("EXT4_DIR_FAIL", "write in subdir");
    if (fsync(fd) != 0)
        return fail("EXT4_DIR_FAIL", "fsync in subdir");
    if (lseek(fd, 0, SEEK_SET) != 0)
        return fail("EXT4_DIR_FAIL", "lseek in subdir");
    memset(buf, 0, sizeof(buf));
    if (read(fd, buf, SEED_LEN) != SEED_LEN || memcmp(buf, seed, SEED_LEN) != 0)
        return fail("EXT4_DIR_FAIL", "readback in subdir");
    close(fd);
    printf("EXT4_DIR_PASS\n");

    printf("EXT4_PASS\n");
    return 0;
}
