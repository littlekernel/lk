#pragma   once
#include  <platform/zynq.h>

struct pktbuf;

typedef void (*gem_cb_t)(struct pktbuf *p);
status_t gem_init(uintptr_t regsbase);
void gem_set_callback(gem_cb_t rx);
void gem_set_macaddr(uint8_t mac[6]);
int gem_send_raw_pkt(struct pktbuf *p);

void gem_disable(void);

struct gem_regs {
    uint32_t net_ctrl;
    uint32_t net_cfg;
    uint32_t net_status;
    uint32_t ___reserved1;
    uint32_t dma_cfg;
    uint32_t tx_status;
    uint32_t rx_qbar;
    uint32_t tx_qbar;
    uint32_t rx_status;
    uint32_t intr_status;
    uint32_t intr_en;
    uint32_t intr_dis;
    uint32_t intr_mask;
    uint32_t phy_maint;
    uint32_t rx_pauseq;
    uint32_t tx_pauseq;
    uint32_t ___reserved2[16];
    uint32_t hash_bot;
    uint32_t hash_top;
    uint32_t spec_addr1_bot;
    uint32_t spec_addr1_top;
    uint32_t spec_addr2_bot;
    uint32_t spec_addr2_top;
    uint32_t spec_addr3_bot;
    uint32_t spec_addr3_top;
    uint32_t spec_addr4_bot;
    uint32_t spec_addr4_top;
    uint32_t type_id_match1;
    uint32_t type_id_match2;
    uint32_t type_id_match3;
    uint32_t type_id_match4;
    uint32_t wake_on_lan;
    uint32_t ipg_stretch;
    uint32_t stacked_vlan;
    uint32_t tx_pfc_pause;
    uint32_t spec_addr1_mask_bot;
    uint32_t spec_addr1_mask_top;
    uint32_t ___reserved3[11];
    uint32_t module_id;
    uint32_t octets_tx_bot;
    uint32_t octets_tx_top;
    uint32_t frames_tx;
    uint32_t broadcast_frames_tx;
    uint32_t multi_frames_tx;
    uint32_t pause_frames_tx;
    uint32_t frames_64b_tx;
    uint32_t frames_65to127b_tx;
    uint32_t frames_128to255b_tx;
    uint32_t frames_256to511b_tx;
    uint32_t frames_512to1023b_tx;
    uint32_t frames_1024to1518b_tx;
    uint32_t ___reserved4;
    uint32_t tx_under_runs;
    uint32_t single_collisn_frames;
    uint32_t multi_collisn_frames;
    uint32_t excessive_collisns;
    uint32_t late_collisns;
    uint32_t deferred_tx_frames;
    uint32_t carrier_sense_errs;
    uint32_t octets_rx_bot;
    uint32_t octets_rx_top;
    uint32_t frames_rx;
    uint32_t bdcast_fames_rx;
    uint32_t multi_frames_rx;
    uint32_t pause_rx;
    uint32_t frames_64b_rx;
    uint32_t frames_65to127b_rx;
    uint32_t frames_128to255b_rx;
    uint32_t frames_256to511b_rx;
    uint32_t frames_512to1023b_rx;
    uint32_t frames_1024to1518b_rx;
    uint32_t ___reserved5;
    uint32_t undersz_rx;
    uint32_t oversz_rx;
    uint32_t jab_rx;
    uint32_t fcs_errors;
    uint32_t length_field_errors;
    uint32_t rx_symbol_errors;
    uint32_t align_errors;
    uint32_t rx_resource_errors;
    uint32_t rx_overrun_errors;
    uint32_t ip_hdr_csum_errors;
    uint32_t tcp_csum_errors;
    uint32_t udp_csum_errors;
    uint32_t ___reserved6[7];
    uint32_t timer_strobe_s;
    uint32_t timer_strobe_ns;
    uint32_t timer_s;
    uint32_t timer_ns;
    uint32_t timer_adjust;
    uint32_t timer_incr;
    uint32_t ptp_tx_s;
    uint32_t ptp_tx_ns;
    uint32_t ptp_rx_s;
    uint32_t ptp_rx_ns;
    uint32_t ptp_peer_tx_s;
    uint32_t ptp_peer_tx_ns;
    uint32_t ptp_peer_rx_s;
    uint32_t ptp_peer_rx_ns;
    uint32_t ___reserved7[22];
    uint32_t design_cfg2;
    uint32_t design_cfg3;
    uint32_t design_cfg4;
    uint32_t design_cfg5;
};

/*      net_ctrl                             */
#define NET_CTRL_LOOP_EN                     (1 <<  1)
#define NET_CTRL_RX_EN                       (1 <<  2)
#define NET_CTRL_TX_EN                       (1 <<  3)
#define NET_CTRL_MD_EN                       (1 <<  4)
#define NET_CTRL_STATCLR                     (1 <<  5)
#define NET_CTRL_STATINC                     (1 <<  6)
#define NET_CTRL_STATW_EN                    (1 <<  7)
#define NET_CTRL_BACK_PRESSURE               (1 <<  8)
#define NET_CTRL_START_TX                    (1 <<  9)
#define NET_CTRL_HALT_TX                     (1 <<  10)
#define NET_CTRL_PAUSE_TX                    (1 <<  11)
#define NET_CTRL_ZERO_PAUSE_TX               (1 <<  12)
#define NET_CTRL_STR_RX_TIMESTAMP            (1 <<  15)
#define NET_CTRL_EN_PFC_PRI_PAUSE_RX         (1 <<  16)
#define NET_CTRL_TX_PFC_PRI_PAUSE_FRAME      (1 <<  17)
#define NET_CTRL_FLUSH_NEXT_RX_DPRAM_PKT     (1 <<  18)
/*      net_cfg                              */
#define NET_CFG_SPEED_100                    (1)
#define NET_CFG_FULL_DUPLEX                  (1 <<  1)
#define NET_CFG_DISC_NON_VLAN                (1 <<  2)
#define NET_CFG_COPY_ALL                     (1 <<  4)
#define NET_CFG_NO_BCAST                     (1 <<  5)
#define NET_CFG_MULTI_HASH_EN                (1 <<  6)
#define NET_CFG_UNI_HASH_EN                  (1 <<  7)
#define NET_CFG_RX_1536_BYTE                 (1 <<  8)
#define NET_CFG_EXT_ADDR_MATCH_EN            (1 <<  9)
#define NET_CFG_GIGE_EN                      (1 <<  10)
#define NET_CFG_PCS_SEL                      (1 <<  11)
#define NET_CFG_RETRY_TEST                   (1 <<  12)
#define NET_CFG_PAUSE_EN                     (1 <<  13)
#define NET_CFG_RX_BUF_OFFSET(x)             (x <<  14)
#define NET_CFG_LEN_ERR_FRAME_DISC           (1 <<  16)
#define NET_CFG_FCS_REMOVE                   (1 <<  17)
#define NET_CFG_MDC_CLK_DIV(x)               (x <<  18)
#define NET_CFG_DBUS_WIDTH(x)                (x <<  21)
#define NET_CFG_DIS_CP_PAUSE_FRAME           (1 <<  23)
#define NET_CFG_RX_CHKSUM_OFFLD_EN           (1 <<  24)
#define NET_CFG_RX_HD_WHILE_TX               (1 <<  25)
#define NET_CFG_IGNORE_RX_FCS                (1 <<  26)
#define NET_CFG_SGMII_EN                     (1 <<  27)
#define NET_CFG_IPG_STRETCH_EN               (1 <<  28)
#define NET_CFG_RX_BAD_PREAMBLE              (1 <<  29)
#define NET_CFG_IGNORE_IPG_RX_ER             (1 <<  30)
#define NET_CFG_UNIDIR_EN                    (1 <<  31)
/*      net_status                           */
#define NET_STATUS_PFC_PRI_PAUSE_NEG         (1 <<  6)
#define NET_STATUS_PCS_AUTONEG_PAUSE_TX_RES  (1 <<  5)
#define NET_STATUS_PCS_AUTONEG_PAUSE_RX_RES  (1 <<  4)
#define NET_STATUS_PCS_AUTONEG_DUP_RES       (1 <<  3)
#define NET_STATUS_PHY_MGMT_IDLE             (1 <<  2)
#define NET_STATUS_MDIO_IN_PIN_STATUS        (1 <<  1)
#define NET_STATUS_PCS_LINK_STATE            (1 <<  0)
/*      dma_cfg                              */
#define DMA_CFG_AHB_FIXED_BURST_LEN(x)       (x)
#define DMA_CFG_AHB_ENDIAN_SWP_MGM           (1 <<  6)
#define DMA_CFG_AHB_ENDIAN_SWP_PKT_EN        (1 <<  7)
#define DMA_CFG_RX_PKTBUF_MEMSZ_SEL(x)       (x <<  8)
#define DMA_CFG_TX_PKTBUF_MEMSZ_SEL          (1 <<  10)
#define DMA_CFG_CSUM_GEN_OFFLOAD_EN          (1 <<  11)
#define DMA_CFG_AHB_MEM_RX_BUF_SIZE(x)       (x <<  16)
#define DMA_CFG_DISC_WHEN_NO_AHB             (1 <<  24)

/* tx descriptor */
#define TX_BUF_LEN(x)                        (x & 0x3FFF)
#define TX_LAST_BUF                          (1 << 15)
#define TX_CHKSUM_GEN_ERR(x)                 ((x >> 20) & 0x7)
#define TX_NO_CRC                            (1 << 16)
#define TX_LATE_COLLISION                    (1 << 26)
#define TX_RETRY_EXCEEDED                    (1 << 29)
#define TX_DESC_WRAP                         (1 << 30)
#define TX_DESC_USED                         (1 << 31)

/* rx descriptor */
#define RX_DESC_USED                         (1 << 0)
#define RX_DESC_WRAP                         (1 << 1)
#define RX_BUF_LEN(x)                        (x & 0x1FFF)
#define RX_FCS_BAD_FCS                       (1 << 13)
#define RX_START_OF_FRAME                    (1 << 14)
#define RX_END_OF_FRAME                      (1 << 15)
#define RX_CFI                               (1 << 16)
#define RX_VLAN_PRIO(x)                      ((x >> 17) & 0x7)
#define RX_PRIO_TAG                          (1 << 20)
#define RX_VLAN_DETECT                       (1 << 21)
#define RX_CHKSUM_MATCH(x)                   ((1 >> 22) & 0x3)
#define RX_SNAP_ENCODED                      (1 << 24)
#define RX_ADDR_REG_MATCH(x)                 ((1 >> 25) & 0x3)
#define RX_SPECIFIC_ADDR_MATCH               (1 << 27)
#define RX_EXT_MATCH                         (1 << 28)
#define RX_UNICAST_MATCH                     (1 << 30)
#define RX_MULTICAST_MATCH                   (1 << 31)

#define INTR_MGMT_SENT                       (1 << 0)
#define INTR_RX_COMPLETE                     (1 << 1)
#define INTR_RX_USED_READ                    (1 << 2)
#define INTR_TX_USED_READ                    (1 << 3)
#define INTR_RETRY_EX                        (1 << 5)
#define INTR_TX_CORRUPT                      (1 << 6)
#define INTR_TX_COMPLETE                     (1 << 7)
#define INTR_RX_OVERRUN                      (1 << 10)
#define INTR_HRESP_NOT_OK                    (1 << 11)

#define TX_STATUS_USED_READ                  (1)
#define TX_STATUS_COLLISION                  (1 << 1)
#define TX_STATUS_RETRY_LIMIT                (1 << 2)
#define TX_STATUS_GO                         (1 << 3)
#define TX_STATUS_CORR_AHB                   (1 << 4)
#define TX_STATUS_COMPLETE                   (1 << 5)
#define TX_STATUS_UNDER_RUN                  (1 << 6)
#define TX_STATUS_LATE_COLLISION             (1 << 7)
#define TX_STATUS_HRESP_NOT_OK               (1 << 8)

#define RX_STATUS_BUFFER_NOT_AVAIL           (1)
#define RX_STATUS_FRAME_RECD                 (1 << 1)
#define RX_STATUS_RX_OVERRUN                 (1 << 2)
#define RX_STATUS_HRESP_NOT_OK               (1 << 3)
