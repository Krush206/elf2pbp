#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#include "paramsfo.h"
#include "icon0png.h"
#include "icon1pmf.h"
#include "pict1png.h"
#include "icon1png.h"
#include "snd0at3.h"
#include "datapsar.h"

#define PBP_VERSION 0x00000100
#define DEFAULT_APP_TITLE "PSPDev Application"

typedef struct {
    char magic[4];
    s32 version;
    s32 offsetParamSfo;
    s32 offsetIcon0Png;
    s32 offsetIcon1Pmf;
    s32 offsetPic0Png;
    s32 offsetPic1Png;
    s32 offsetSnd0At3;
    s32 offsetDataPsp;
    s32 offsetDataPsar;
} PbpHeader;

typedef struct {
    s16 offsetString;
    u16 flags;
    s32 size;
    s32 sizeBuffer;
    s32 offsetData;
} SfoItem;

typedef struct {
    char magic[4];
    s32 version;
    s32 offsetStrings;
    s32 offsetData;
    s32 numItems;
    SfoItem items[1];
} SfoHeader;

static s32 patchSfoTitle(u8 *bufferParamSfo, s32 sizeParamSfo, const char *appTitle)
{
    SfoHeader *header = (SfoHeader *)bufferParamSfo;
    for (s32 i = 0; i < header->numItems; i++) {
        char *name = (char *)bufferParamSfo + header->offsetStrings + header->items[i].offsetString;
        if (strcmp(name, "TITLE") == 0) {
            s32 newlen = (s32)strlen(appTitle);
            if (newlen > header->items[i].sizeBuffer) newlen = header->items[i].sizeBuffer;
            header->items[i].size = newlen;
            memcpy(bufferParamSfo + header->offsetData + header->items[i].offsetData, appTitle, (size_t)newlen);
            if (newlen < header->items[i].sizeBuffer) {
                memset(bufferParamSfo + header->offsetData + header->items[i].offsetData + newlen, 0, (size_t)(header->items[i].sizeBuffer - newlen));
            }
            break;
        }
    }
    return 0;
}

static s32 generatePbp(FILE *fd, u8 *elfBuffer, s32 elfSize, const char *appTitle)
{
    PbpHeader h;
    h.magic[0] = '\0';
    h.magic[1] = 'P';
    h.magic[2] = 'B';
    h.magic[3] = 'P';
    h.version = PBP_VERSION;

    h.offsetParamSfo = sizeof(PbpHeader);
    h.offsetIcon0Png = h.offsetParamSfo + sizeParamSfo;
    h.offsetIcon1Pmf = h.offsetIcon0Png + sizeIcon0Png;
    h.offsetPic0Png  = h.offsetIcon1Pmf + sizeIcon1Pmf;
    h.offsetPic1Png  = h.offsetPic0Png + sizeIcon1Png;   /* legacy icon1png used as PIC0 */
    h.offsetSnd0At3  = h.offsetPic1Png + sizePict1Png;
    h.offsetDataPsp  = h.offsetSnd0At3 + sizeSnd0At3;
    h.offsetDataPsar = h.offsetDataPsp + elfSize;

    patchSfoTitle(bufferParamSfo, sizeParamSfo, appTitle);

    fwrite(&h, 1, sizeof(h), fd);
    fwrite(bufferParamSfo, 1, sizeParamSfo, fd);
    fwrite(bufferIcon0Png, 1, sizeIcon0Png, fd);
    fwrite(bufferIcon1Pmf, 1, sizeIcon1Pmf, fd);
    fwrite(bufferIcon1Png, 1, sizeIcon1Png, fd);   /* PIC0 */
    fwrite(bufferPict1Png, 1, sizePict1Png, fd);   /* PIC1 */
    fwrite(bufferSnd0At3,  1, sizeSnd0At3,  fd);
    fwrite(elfBuffer,      1, elfSize,      fd);
    fwrite(bufferDataPsar, 1, sizeDataPsar, fd);
    return 0;
}

int main(int argc, char *argv[])
{
    const char *title = "Elf2Pbp C v1.0 6.61 compatible";
    const char *usage = "usage: Elf2Pbp <elf filename> [app title]\n";

    if (argc < 2 || argc > 3) {
        printf("%s\n%s\n", title, usage);
        return 1;
    }

    char appTitle[256];
    if (argc == 3) {
        strncpy(appTitle, argv[2], 255);
        appTitle[255] = '\0';
    } else {
        strncpy(appTitle, DEFAULT_APP_TITLE, 255);
        appTitle[255] = '\0';
    }

    FILE *elf_fd = fopen(argv[1], "rb");
    if (!elf_fd) {
        printf("error opening %s\n", argv[1]);
        return 2;
    }
    fseek(elf_fd, 0, SEEK_END);
    long elfSize = ftell(elf_fd);
    fseek(elf_fd, 0, SEEK_SET);
    u8 *elfBuffer = malloc((size_t)elfSize);
    if (!elfBuffer) {
        fclose(elf_fd);
        return 2;
    }
    if (fread(elfBuffer, 1, (size_t)elfSize, elf_fd) != (size_t)elfSize) {
        free(elfBuffer);
        fclose(elf_fd);
        return 2;
    }
    fclose(elf_fd);

    FILE *pbp_fd = fopen("EBOOT.PBP", "wb");
    if (!pbp_fd) {
        free(elfBuffer);
        printf("error creating EBOOT.PBP\n");
        return 2;
    }
    generatePbp(pbp_fd, elfBuffer, (s32)elfSize, appTitle);
    fclose(pbp_fd);
    free(elfBuffer);
    printf("successfully generated EBOOT.PBP\n");
    return 0;
}
