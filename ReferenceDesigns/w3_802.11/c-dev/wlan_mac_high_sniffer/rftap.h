#include <stdint.h>

static const uint8_t RFTAP_IP_DEFAULT[] = { 192, 168, 100, 1 };

struct rftap_hdr {
    uint32_t magic;  // signature: "RFta"
    uint32_t len32;  // length
    uint16_t flags;  // bitfield indicating presence of fields
    uint8_t *data;  // the rftap fields
} __attribute__((packed));
typedef struct rftap_hdr rftap_hdr_t;

void rftap_frame_init(rftap_hdr_t *frame);
void rftap_init(unsigned int device_num);
void rftap_send(void);
