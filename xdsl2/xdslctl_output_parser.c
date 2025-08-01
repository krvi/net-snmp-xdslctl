#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "xdslctl_output_parser.h"

const char* c_path_to_xdslctl = "/bin/";
const char* c_xdslctl = "xdslctl";

static void trim(char* str)
{
	char* p = str;
	while (isspace(*p)) p++;
	memmove(str, p, strlen(p) + 1);

	for (int i = strlen(str) - 1; i >= 0 && isspace(str[i]); i--) {
		str[i] = '\0';
	}
}

static void parse_stat_line(char* line, xdsl_stats_t* stats, uint8_t* current_bearer)
{
	DEBUGMSGTL(("xdslctl_output_parser:parse_stat_line", "Raw line: '%s'\n", line));
	while (isspace((unsigned char)*line)) line++;  // skip leading whitespace

	if (strncmp(line, "Max:", 4) == 0) {
		sscanf(line, "Max: Upstream rate = %d Kbps, Downstream rate = %d Kbps", &stats->max_up_rate_kbps, &stats->max_down_rate_kbps);
	}
	else if (strstr(line, "Bearer:") != NULL) {
		uint8_t bearer = 0;
		uint32_t up = 0, down = 0;
		sscanf(line, "Bearer: %hhu, Upstream rate = %u Kbps, Downstream rate = %u Kbps", &bearer, &up, &down);
		stats->bearer[bearer].up_rate_kbps = up;
		stats->bearer[bearer].down_rate_kbps = down;
		*current_bearer = bearer;
	}
	else if (strstr(line, "Bearer ") != NULL) {
		uint8_t bearer = 0;
		sscanf(line, "Bearer %hhu", &bearer);
		*current_bearer = bearer;
	}
	else if (*current_bearer < 2) {
		// Assuming there are only 2 bearers (0 and 1)
		if (strncmp(line, "INP:", 4) == 0) {
			sscanf(line, "INP: %f %f", &stats->bearer[*current_bearer].inp[0], &stats->bearer[*current_bearer].inp[1]);
		}
		else if (strncmp(line, "INPRein:", 8) == 0) {
			sscanf(line, "INPRein: %f %f", &stats->bearer[*current_bearer].inp_rein[0], &stats->bearer[*current_bearer].inp_rein[1]);
		}
		else if (strncmp(line, "delay:", 6) == 0) {
			sscanf(line, "delay: %u %u", &stats->bearer[*current_bearer].delay[0], &stats->bearer[*current_bearer].delay[1]);
		}
		else if (strncmp(line, "PER:", 4) == 0) {
			sscanf(line, "PER: %f %f", &stats->bearer[*current_bearer].per[0], &stats->bearer[*current_bearer].per[1]);
		}
		else if (strncmp(line, "OR:", 3) == 0) {
			sscanf(line, "OR: %f %f", &stats->bearer[*current_bearer].output_rate[0], &stats->bearer[*current_bearer].output_rate[1]);
		}
		else if (strncmp(line, "AgR:", 4) == 0) {
			sscanf(line, "AgR: %f %f", &stats->bearer[*current_bearer].agg_rate[0], &stats->bearer[*current_bearer].agg_rate[1]);
		}
	}
	if (strstr(line, "SNR (dB):")) {
		sscanf(line, "SNR (dB): %f %f", &stats->snr_db[0], &stats->snr_db[1]);
	}
	else if (strstr(line, "Attn(dB):")) {
		sscanf(line, "Attn(dB): %f %f", &stats->attn_db[0], &stats->attn_db[1]);
	}
	else if (strstr(line, "Pwr(dBm):")) {
		sscanf(line, "Pwr(dBm): %f %f", &stats->pwr_dbm[0], &stats->pwr_dbm[1]);
	}
	else if (strncmp(line, "FEC:", 4) == 0) {
		sscanf(line, "FEC: %llu %llu", &stats->fec[0], &stats->fec[1]);
	}
	else if (strncmp(line, "CRC:", 4) == 0) {
		sscanf(line, "CRC: %llu %llu", &stats->crc[0], &stats->crc[1]);
	}
	else if (strncmp(line, "ES:", 3) == 0) {
		sscanf(line, "ES: %llu %llu", &stats->es[0], &stats->es[1]);
	}
	else if (strncmp(line, "SES:", 4) == 0) {
		sscanf(line, "SES: %llu %llu", &stats->ses[0], &stats->ses[1]);
	}
	else if (strncmp(line, "UAS:", 4) == 0) {
		sscanf(line, "UAS: %llu %llu", &stats->uas[0], &stats->uas[1]);
	}
	else if (strncmp(line, "Status:", 7) == 0) {
		sscanf(line, "Status: %31s", stats->status);
	}
	else if (strncmp(line, "Link Power State:", 17) == 0) {
		char state_str[8] = { 0 };
		sscanf(line, "Link Power State: %7s", state_str);
		if (strcmp(state_str, "L0") == 0) {
			stats->xdsl2LineStatusPwrMngState = L0;
		}
		else if (strcmp(state_str, "L1") == 0) {
			stats->xdsl2LineStatusPwrMngState = L1;
		}
		else if (strcmp(state_str, "L2") == 0) {
			stats->xdsl2LineStatusPwrMngState = L2;
		}
		else if (strcmp(state_str, "L3") == 0) {
			stats->xdsl2LineStatusPwrMngState = L3;
		}
	}
	else if (strncmp(line, "VDSL2 Profile:", 14) == 0) {
		sscanf(line, "VDSL2 Profile:%*s %7s", stats->profile);
	}
	else if (strncmp(line, "Trellis:", 8) == 0) {
		stats->trellis[0] = strstr(line, "U:ON") != NULL;
		stats->trellis[1] = strstr(line, "D:ON") != NULL;
	}
}

static void parse_vendor_line(char* line, xdsl_vendor_info_t* vendor_info)
{
	if (strncmp(line, "ChipSet Vendor Id:", 18) == 0) {
		sscanf(line, "ChipSet Vendor Id: %8[^:]:0x%x", vendor_info->xdsl2LInvG994VendorId, vendor_info->xdsl2LInvSystemVendorId);
	}
	else if (strncmp(line, "ChipSet VersionNumber:", 22) == 0) {
		sscanf(line, "ChipSet VersionNumber: 0x%x", vendor_info->xdsl2LInvVersionNumber);
	}
	else if (strncmp(line, "ChipSet SerialNumber:", 21) == 0) {
		sscanf(line, "ChipSet SerialNumber: %31s", vendor_info->xdsl2LInvSerialNumber);
	}
}

static int parse_stats_from_cmd(const char* cmd, xdsl_stats_t* stats)
{
	DEBUGMSGTL(("xdslctl_output_parser", "About to run: %s .\n", cmd));
	FILE* fp = popen(cmd, "r");
	if (fp == NULL) {
		snmp_log(LOG_ERR, "Failed to run command: %s", cmd);
		return -1;
	}
	
	char buf[MAX_LINE_LEN];
	uint8_t current_bearer = 0;

	while (fgets(buf, sizeof(buf), fp)) {
		trim(buf);
		if (strlen(buf) == 0)
			continue;
		parse_stat_line(buf, stats, &current_bearer);
	}

	if (pclose(fp) != 0) {
		snmp_log(LOG_ERR, "Failed to close command pipe");
		return -1;
	}
	return 0;
}

static int parse_vendor_info(const char* cmd, xdsl_vendor_info_t* vendor_info)
{
	DEBUGMSGTL(("xdslctl_output_parser", "About to run: %s .\n", cmd));
	FILE* fp = popen(cmd, "r");
	if (fp == NULL) {
		return -1;
	}

	char buf[MAX_LINE_LEN];

	while (fgets(buf, sizeof(buf), fp)) {
		trim(buf);
		if (strlen(buf) == 0)
			continue;
		parse_vendor_line(buf, vendor_info);
	}

	if (pclose(fp) != 0) {
		return -1;
	}
	DEBUGMSGTL(("xdslctl_output_parser", "Finished: %s .\n", cmd));
	return 0;
}

uint8_t get_xdslctl_stats(xdsl_stats_t* stats_array, uint8_t max_lines)
{
	static xdsl_stats_t cached_stats[MAX_XDSL_LINES];
    static uint8_t cached_count = 0;
	static time_t last_fetch = 0;

	time_t now = time(NULL);
	DEBUGMSGTL(("get_xdslctl_stats", "Time now: %lld, Last fetch: %lld\n", now, last_fetch));
	if (difftime(now, last_fetch) < 3.0) {
		DEBUGMSGTL(("get_xdslctl_stats", "Returning cached copy.\n"));
		// Use cached copy
		memcpy(stats_array, cached_stats, sizeof(xdsl_stats_t) * cached_count);
		return cached_count;
	};

	const char* c_params = "info --stats";
	
	uint8_t lines_found = 0;
	char c_command[128], c_xdslctl_full_path[64];

	if (max_lines > 1) {
		for (uint8_t i = 0; i < max_lines; i++) {
			// Check for bonding mode via presence of xdslctl0..i
			snprintf(c_xdslctl_full_path, sizeof(c_xdslctl_full_path), "%s%s%d", c_path_to_xdslctl, c_xdslctl, i);
			if (access(c_xdslctl_full_path, X_OK) != 0)
				break;
			snprintf(c_command, sizeof(c_command), "%s%d %s", c_xdslctl, i, c_params);
			if (parse_stats_from_cmd(c_command, &cached_stats[i]) == 0) {
				cached_stats[i].line_id = i;
				lines_found++;
			}

			DEBUGMSGTL(("get_xdslctl_stats", "Line %d bearer0 up=%u down=%u\n",
				i,
				cached_stats[i].bearer[0].up_rate_kbps,
				cached_stats[i].bearer[0].down_rate_kbps));
		}
	}
	snprintf(c_xdslctl_full_path, sizeof(c_xdslctl_full_path), "%s%s", c_path_to_xdslctl, c_xdslctl);
	if (access(c_xdslctl_full_path, X_OK) == 0) {
		if (max_lines > 0 && lines_found == 0) {
			// Single-line
			snprintf(c_command, sizeof(c_command), "%s %s", c_xdslctl, c_params);
			if (parse_stats_from_cmd(c_command, &cached_stats[0]) == 0) {
				cached_stats[0].line_id = 0;
				lines_found++;
			}
		}
	}

	last_fetch = now;
	cached_count = lines_found;
	memcpy(stats_array, cached_stats, sizeof(xdsl_stats_t) * lines_found);
	DEBUGMSGTL(("get_xdslctl_stats", "Found %d lines\n", lines_found));
	return lines_found;
}

int get_xdslctl_vendor_info(xdsl_vendor_info_t* vendor_info)
{
	static xdsl_vendor_info_t cached_vendor_info;
	uint8_t cached = 0;
	const char* params = "info --vendor";
	char c_command[128], c_full_path[64];

	// Use cached vendor info if still valid
	if (cached > 0) {
		memcpy(vendor_info, &cached_vendor_info, sizeof(xdsl_vendor_info_t));
		DEBUGMSGTL(("get_xdslctl_vendor_info", "Served cached vendor info"));
		return 0;
	}
	snprintf(c_full_path, sizeof(c_full_path), "%s%s", c_path_to_xdslctl, c_xdslctl);
	if (access(c_full_path, X_OK) != 0) {
		DEBUGMSGTL(("get_xdslctl_vendor_info", "Could not run or access: %s", c_full_path));
		return -1;
	}
	snprintf(c_command, sizeof(c_command), "%s %s", c_xdslctl, params);
	if (parse_vendor_info(c_command, &cached_vendor_info) == 0) {
		memcpy(vendor_info, &cached_vendor_info, sizeof(xdsl_vendor_info_t));
		cached = 1;
		DEBUGMSGTL(("get_xdslctl_vendor_info", "Served new vendor info"));
		return 0;
	}
	return -1;
}