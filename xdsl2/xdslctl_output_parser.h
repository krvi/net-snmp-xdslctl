#ifndef XDSLCTL_OUTPUT_PARSER_H
#define XDSLCTL_OUTPUT_PARSER_H

#include <stdint.h>

#define MAX_XDSL_LINES 8
#define MAX_LINE_LEN 256
#define BASE_IFINDEX 13 // On VMG4005, TODO: Check net-snmpd for proper ifindex ? ptm0, ptm0.2 ? atm?

typedef enum { // ITU-T G.997.1, paragraph #7.5.1.5
    L0 = 0,         // Full power / Showtime
	L1,             // Low power
    L2,             // Low power
    L3              // No power
} xdsl_line_power_management_state_t;

typedef enum {
    Unknown = 0,
    xtuc,
    xtur
} xdsl_termination_unit_t;

typedef enum {
    upstream = 1,
    downstream = 2
} xdsl_transmission_direction_t;

typedef enum { // ITU-T G.997.1, paragraph #7.5.1.6
    Successful = 0,
    Configuration_Error,
    Configuration_Not_Feasible,
    Communication_Problem,
    No_Peer_xTU_Detected,
    Unknown_Or_Other_Failure_Cause,
    G998_4_Retransmission_Mode_Not_Selected
} xdsl_initialization_result_cause_t;

typedef struct {
    uint32_t valid_intervals;
    uint32_t invalid_intervals;
    uint32_t time_elapsed_sec;     // seconds
    uint64_t fec[2];
    uint64_t crc[2];
    uint64_t es[2];
    uint64_t ses[2];
    uint64_t uas[2];
} xdsl_interval_stats_t;

typedef struct {
    uint32_t up_rate_kbps;
    uint32_t down_rate_kbps;

    // INP
    float inp[2];
    float inp_rein[2];
    uint32_t delay[2];
    float per[2];
    float output_rate[2];
    float agg_rate[2];
} xdsl_bearer_stats_t;


typedef struct {
    uint8_t line_id;
    char status[32];                // "Showtime", etc. state enums?

    // Basic line rates
    uint32_t max_up_rate_kbps;
    uint32_t max_down_rate_kbps;
    xdsl_bearer_stats_t bearer[2];

    // Signal quality
    float snr_db[2];               // Down, Up
    float attn_db[2];              // Down, Up
    float pwr_dbm[2];              // Down, Up

    // Error counters (since link up)
    uint64_t fec[2];               // Forward Error Correction
    uint64_t crc[2];
    uint64_t es[2];                // Errore Seconds
    uint64_t ses[2];               // Severe Error Seconds
    uint64_t uas[2];               // Unavailable Seconds

    xdsl_interval_stats_t stats_15m;
    xdsl_interval_stats_t stats_1d;

    // Total time (link uptime)
    uint16_t link_uptime_days;
    uint32_t link_uptime_secs;     // seconds modulo 1 day

    // Retrains and errors
    uint32_t retrains;
    uint32_t failed_retrains;
    uint32_t failed_fast_retrains;

    // G.INP
    uint32_t rtx_tx[2];
    uint32_t rtx_c[2];
    uint32_t rtx_uc[2];
    uint32_t leftrs[2];
    uint32_t min_eftr[2];
    uint64_t err_free_bits[2];

    // INP/delay
    float inp[2];
    float inp_rein[2];
    uint32_t delay[2];
    float per[2];
    float output_rate[2];
    float aggregate_rate[2];

    // Bitswap
    uint32_t bitswap_tx[2];
    uint32_t bitswap_rx[2];

    // Optional fields
    char mode[16];              // "VDSL2 Annex B"
    char profile[4];            // "17a"
    uint8_t trellis[2];         // Ds and Us
    xdsl_line_power_management_state_t xdsl2LineStatusPwrMngState;

} xdsl_stats_t;

typedef struct {
    char xdsl2LInvG994VendorId[8];
    char xdsl2LInvSystemVendorId[8];
    char xdsl2LInvVersionNumber[16];
    char xdsl2LInvSerialNumber[32];
} xdsl_vendor_info_t;

uint8_t get_xdslctl_stats(xdsl_stats_t* stats_array, uint8_t max_lines);
int get_xdslctl_vendor_info(xdsl_vendor_info_t* vendor_info);

#endif // XDSLCTL_OUTPUT_PARSER_H