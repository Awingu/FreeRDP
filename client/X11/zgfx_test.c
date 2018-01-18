#include <freerdp/codec/zgfx.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <netinet/in.h>

struct _ZGFX_CONTEXT32
{
    BOOL Compressor;

    const UINT32 pbInputCurrent;
    const UINT32 pbInputEnd;

    UINT32 bits;
    UINT32 cBitsRemaining;
    UINT32 BitsCurrent;
    UINT32 cBitsCurrent;

    BYTE OutputBuffer[65536];
    UINT32 OutputCount;

    BYTE HistoryBuffer[2500000];
    UINT32 HistoryIndex;
    UINT32 HistoryBufferSize;
};
typedef struct _ZGFX_CONTEXT32 ZGFX_CONTEXT32;
static uint8_t context[] = {

};

typedef struct Header {
    uint16_t cmd;
    uint16_t flags;
    uint32_t length;
} Header;

int main(int argc, const char ** argv) {
    ZGFX_CONTEXT * context = zgfx_context_new(FALSE);
    BYTE * dst_data = NULL;
    UINT32 dst_size = 0;
    ZGFX_CONTEXT32 context32;
    printf("sizeof(ZGFX_CONTEXT32) = %zu\nsizeof(ZGFX_CONTEXT) = %zu\n", sizeof(ZGFX_CONTEXT32), sizeof(ZGFX_CONTEXT));
    printf("offsetof(ZGFX_CONTEXT32, HistoryBufferSize) = %zu\n", offsetof(ZGFX_CONTEXT32, HistoryBufferSize));
    printf("offsetof(ZGFX_CONTEXT, HistoryBufferSize) = %zu\n", offsetof(ZGFX_CONTEXT, HistoryBufferSize));


    size_t buffer_size = 1000000;
    BYTE * buffer = malloc(buffer_size);
    BYTE * buffer_prev = malloc(buffer_size);
    UINT32 message_size = 0;
    UINT32 message_size_prev = 0;

    FILE * f = fopen("/home/rubendv/sandbox/rail/channel_6_Microsoft::Windows::RDS::Graphics_failing_2", "r");
    int zgfx_i = 0;
    int i = 0;
    while(!(feof(f) || ferror(f))) {
        i++;
        fprintf(stderr, "%d frames (%d zgfx) loaded\n", i, zgfx_i);
        size_t items_read = fread(&message_size, 4, 1, f);
        if(items_read != 1) {
            fprintf(stderr, "Unable to read message_size: %zu\n", items_read);
            exit(1);
        }
        message_size = ntohl(message_size);
        assert(message_size < buffer_size);
        items_read = fread(buffer, 1, message_size, f);
        if(items_read != message_size) {
            fprintf(stderr, "Unable to read %u bytes: %zu read instead.\n", message_size, items_read);
            exit(1);
        }
        assert(message_size >= 7);
        uint16_t ws_message_type = *(uint16_t *)(&buffer[0]);
        if(ws_message_type != 0x40) {
            fprintf(stderr, "Ignoring ws_message_type=%x\n", ws_message_type);
            continue;
        }
        int message_type = buffer[2];
        if(message_type != 0x3) {
            fprintf(stderr, "Ignoring message_type=%x\n", ws_message_type);
            continue;
        }
        uint32_t channel_id = *(uint32_t *)(&buffer[3]);
        zgfx_i++;
        assert(buffer[7] == 0xe0 || buffer[7] == 0xe1);
        int status = zgfx_decompress(context, &buffer[7], message_size - 7, &dst_data, &dst_size, 0);
        if(status < 0) {
            fprintf(stderr, "Failed to decompress message %d, status: %d", i, status);
            exit(1);
        }

        int dst_i = 0;
        while(dst_i < dst_size) {
            assert(dst_size - dst_i >= sizeof(Header));
            Header *header = (Header *)&dst_data[dst_i];
            printf("cmd: %02x, flags: %02x, length: %u\n", header->cmd, header->flags, header->length);
            assert(header->flags == 0);
            assert(header->cmd >= 0x1 && header->cmd <= 0x15);
            assert(header->length < 1000000);
            dst_i += header->length;
        }

        memcpy(buffer_prev, buffer, message_size);
        message_size_prev = message_size;

        free(dst_data);
    }
}