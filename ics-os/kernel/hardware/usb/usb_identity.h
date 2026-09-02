#ifndef ICSOS_USB_IDENTITY_H
#define ICSOS_USB_IDENTITY_H

#define USB_VOLUME_ID_MAX 32
#define USB_MEDIA_VOLUME_MAX 4

#define USB_VOLUME_FAT16   1
#define USB_VOLUME_FAT32   2
#define USB_VOLUME_EXFAT   3
#define USB_VOLUME_EXT4    4
#define USB_VOLUME_ISO9660 5

typedef struct {
    unsigned int startlba;
    unsigned int sectors;
    unsigned char type;
    unsigned char length;
    unsigned char value[USB_VOLUME_ID_MAX];
} usb_volume_identity;

typedef struct {
    int valid;
    int count;
    usb_volume_identity volumes[USB_MEDIA_VOLUME_MAX];
} usb_media_identity;

static inline void usb_volume_identity_init(usb_volume_identity *identity,
                                            unsigned int startlba,
                                            unsigned int sectors)
{
    int i;
    identity->startlba = startlba;
    identity->sectors = sectors;
    identity->type = 0;
    identity->length = 0;
    for (i = 0; i < USB_VOLUME_ID_MAX; i++)
        identity->value[i] = 0;
}

static inline int usb_volume_identity_from_boot(
    const unsigned char *data, usb_volume_identity *identity)
{
    int i;
    int offset;
    if (data[3] == 'E' && data[4] == 'X' && data[5] == 'F' &&
        data[6] == 'A' && data[7] == 'T' && data[8] == ' ' &&
        data[9] == ' ' && data[10] == ' ') {
        identity->type = USB_VOLUME_EXFAT;
        offset = 100;
    } else if (data[510] == 0x55 && data[511] == 0xAA &&
               data[66] == 0x29) {
        identity->type = USB_VOLUME_FAT32;
        offset = 67;
    } else if (data[510] == 0x55 && data[511] == 0xAA &&
               data[38] == 0x29) {
        identity->type = USB_VOLUME_FAT16;
        offset = 39;
    } else {
        return 0;
    }
    identity->length = 4;
    for (i = 0; i < 4; i++)
        identity->value[i] = data[offset + i];
    return 1;
}

static inline int usb_volume_identity_from_ext4(
    const unsigned char *data, usb_volume_identity *identity)
{
    int i;
    if (data[0x38] != 0x53 || data[0x39] != 0xEF)
        return 0;
    identity->type = USB_VOLUME_EXT4;
    identity->length = 16;
    for (i = 0; i < 16; i++)
        identity->value[i] = data[0x68 + i];
    return 1;
}

static inline int usb_volume_identity_from_iso9660(
    const unsigned char *data, usb_volume_identity *identity)
{
    int i;
    if (data[1] != 'C' || data[2] != 'D' || data[3] != '0' ||
        data[4] != '0' || data[5] != '1')
        return 0;
    identity->type = USB_VOLUME_ISO9660;
    identity->length = 32;
    for (i = 0; i < 32; i++)
        identity->value[i] = data[40 + i];
    return 1;
}

static inline int usb_media_identity_equal(const usb_media_identity *left,
                                           const usb_media_identity *right)
{
    int i;
    int j;
    if (!left->valid || !right->valid || left->count != right->count)
        return 0;
    for (i = 0; i < left->count; i++) {
        const usb_volume_identity *a = &left->volumes[i];
        const usb_volume_identity *b = &right->volumes[i];
        if (a->startlba != b->startlba || a->sectors != b->sectors ||
            a->type != b->type || a->length != b->length)
            return 0;
        for (j = 0; j < a->length; j++)
            if (a->value[j] != b->value[j])
                return 0;
    }
    return 1;
}

#endif