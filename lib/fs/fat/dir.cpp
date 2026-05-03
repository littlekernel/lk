/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "dir.h"

#include <ctype.h>
#include <endian.h>
#include <lib/bcache/bcache_block_ref.h>
#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fat_fs.h"
#include "fat_priv.h"
#include "file_iterator.h"

#define LOCAL_TRACE FAT_GLOBAL_TRACE(0)

// structure that represents an open dir handle. holds the offset into the directory
// that is being walked.
struct fat_dir_cookie {
    fat_dir *dir;

    struct list_node node;

    // next directory index offset in bytes, 0xffffffff for EOD
    uint32_t index;
    static const uint32_t index_eod = 0xffffffff;
};

// Convert a UTF-8 path element into UCS-2 code units used by FAT LFN entries.
// Rejects malformed/overlong UTF-8, surrogate code points, and non-BMP code points.
status_t fat_utf8_to_ucs2(const char *utf8, uint16_t *ucs2, size_t max_ucs2_len,
                          size_t *out_ucs2_len) {
    DEBUG_ASSERT(utf8 && ucs2);

    size_t out = 0;
    for (size_t i = 0; utf8[i] != '\0';) {
        uint32_t codepoint = 0;
        uint8_t b0 = static_cast<uint8_t>(utf8[i]);

        if (b0 < 0x80) {
            codepoint = b0;
            i += 1;
        } else if ((b0 & 0xe0) == 0xc0) {
            if (utf8[i + 1] == '\0') {
                return ERR_INVALID_ARGS;
            }
            uint8_t b1 = static_cast<uint8_t>(utf8[i + 1]);
            if ((b1 & 0xc0) != 0x80) {
                return ERR_INVALID_ARGS;
            }

            codepoint = ((b0 & 0x1f) << 6) | (b1 & 0x3f);
            if (codepoint < 0x80) {
                return ERR_INVALID_ARGS;
            }
            i += 2;
        } else if ((b0 & 0xf0) == 0xe0) {
            if (utf8[i + 1] == '\0' || utf8[i + 2] == '\0') {
                return ERR_INVALID_ARGS;
            }
            uint8_t b1 = static_cast<uint8_t>(utf8[i + 1]);
            uint8_t b2 = static_cast<uint8_t>(utf8[i + 2]);
            if ((b1 & 0xc0) != 0x80 || (b2 & 0xc0) != 0x80) {
                return ERR_INVALID_ARGS;
            }

            codepoint = ((b0 & 0x0f) << 12) | ((b1 & 0x3f) << 6) | (b2 & 0x3f);
            if (codepoint < 0x800) {
                return ERR_INVALID_ARGS;
            }
            i += 3;
        } else {
            // FAT LFN stores UCS-2 and cannot represent non-BMP code points.
            return ERR_INVALID_ARGS;
        }

        // reject UTF-8 that would encode surrogate code points
        if (codepoint >= 0xd800 && codepoint <= 0xdfff) {
            return ERR_INVALID_ARGS;
        }
        // reject non-BMP code points, since FAT LFN uses UCS-2 and cannot represent them
        if (codepoint > 0xffff) {
            return ERR_INVALID_ARGS;
        }

        if (out >= max_ucs2_len) {
            return ERR_TOO_BIG;
        }
        ucs2[out++] = static_cast<uint16_t>(codepoint);
    }

    if (out_ucs2_len) {
        *out_ucs2_len = out;
    }
    return NO_ERROR;
}

/* Convert a single UCS-2 code point to UTF-8, writing bytes in reverse order.
 * Returns the number of bytes written (1-3). */
inline size_t ucs2_char_to_utf8(uint16_t c, char out[3]) {

    // TODO: handle > U+FFFF code points if we ever want to support them in FAT LFN.
    if (c < 0x80) {
        out[0] = static_cast<char>(c);
        return 1;
    } else if (c < 0x800) {
        out[0] = static_cast<char>(0xc0 | (c >> 6));
        out[1] = static_cast<char>(0x80 | (c & 0x3f));
        return 2;
    } else {
        out[0] = static_cast<char>(0xe0 | (c >> 12));
        out[1] = static_cast<char>(0x80 | ((c >> 6) & 0x3f));
        out[2] = static_cast<char>(0x80 | (c & 0x3f));
        return 3;
    }
}

// Convert UCS-2 code units (from FAT LFN) to UTF-8.
// Returns NO_ERROR on success, ERR_TOO_BIG if the UTF-8 output buffer is too small.
status_t fat_ucs2_to_utf8(const uint16_t *ucs2, size_t ucs2_len, char *utf8,
                          size_t max_utf8_len, size_t *out_utf8_len) {
    DEBUG_ASSERT(utf8);

    if (ucs2_len == 0) {
        utf8[0] = '\0';
        if (out_utf8_len) {
            *out_utf8_len = 0;
        }
        return NO_ERROR;
    }

    DEBUG_ASSERT(ucs2);

    size_t out = 0;
    for (size_t i = 0; i < ucs2_len; i++) {
        uint16_t c = ucs2[i];
        char utf8_bytes[3];
        size_t nbytes = ucs2_char_to_utf8(c, utf8_bytes);

        if (out + nbytes > max_utf8_len) {
            return ERR_TOO_BIG;
        }
        // write forwards by temporarily offsetting the buffer
        for (size_t j = 0; j < nbytes; j++) {
            utf8[out + j] = utf8_bytes[j];
        }
        out += nbytes;
    }

    utf8[out] = '\0';
    if (out_utf8_len) {
        *out_utf8_len = out;
    }
    return NO_ERROR;
}

namespace {

status_t fat_find_short_file_in_dir_with_offsets(fat_fs *fat, uint32_t starting_cluster,
                                                 const char short_name[11], dir_entry *entry,
                                                 uint32_t *entry_start_offset,
                                                 uint32_t *entry_end_offset);

constexpr size_t kFatMaxLfnChars = 255;
constexpr size_t kFatLfnCharsPerEntry = 13;

constexpr size_t kLfnNameOffsets[kFatLfnCharsPerEntry] = {
    1,
    3,
    5,
    7,
    9,
    14,
    16,
    18,
    20,
    22,
    24,
    28,
    30,
};

uint8_t fat_lfn_sfn_checksum(const uint8_t short_name[11]) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < 11; i++) {
        checksum = static_cast<uint8_t>(((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + short_name[i]);
    }
    return checksum;
}

inline char sanitize_sfn_char(uint8_t c) {
    if (isalnum(c)) {
        return static_cast<char>(toupper(c));
    }
    return '_';
}

void write_lfn_name_part(uint8_t *ent, const uint16_t *ucs2, size_t ucs2_len,
                         size_t sequence) {
    size_t start = (sequence - 1) * kFatLfnCharsPerEntry;
    for (size_t i = 0; i < kFatLfnCharsPerEntry; i++) {
        size_t idx = start + i;
        uint16_t v;
        if (idx < ucs2_len) {
            v = ucs2[idx];
        } else if (idx == ucs2_len) {
            v = 0x0000;
        } else {
            v = 0xffff;
        }
        fat_write16(ent, kLfnNameOffsets[i], v);
    }
}

void fill_lfn_dirent(uint8_t *ent, const uint16_t *ucs2, size_t ucs2_len,
                     uint8_t sequence, bool sequence_is_last, uint8_t checksum) {
    memset(ent, 0, DIR_ENTRY_LENGTH);
    ent[0] = sequence_is_last ? static_cast<uint8_t>(sequence | 0x40) : sequence;
    ent[11] = static_cast<uint8_t>(fat_attribute::lfn);
    ent[12] = 0;
    ent[13] = checksum;
    fat_write16(ent, 26, 0);
    write_lfn_name_part(ent, ucs2, ucs2_len, sequence);
}

void build_short_name_alias(const char *name, uint32_t ordinal, char sfn[12]) {
    memset(sfn, ' ', 11);
    sfn[11] = 0;

    const char *dot = strrchr(name, '.');
    const char *stem_end = dot ? dot : (name + strlen(name));

    char ext[3] = {' ', ' ', ' '};
    if (dot && dot[1] != 0) {
        size_t e = 0;
        for (const char *p = dot + 1; *p && e < sizeof(ext); p++) {
            if (*p == '.') {
                continue;
            }
            ext[e++] = sanitize_sfn_char(static_cast<uint8_t>(*p));
        }
    }

    char suffix[8];
    snprintf(suffix, sizeof(suffix), "~%" PRIu32, ordinal);
    size_t suffix_len = strlen(suffix);
    suffix_len = MIN(suffix_len, 7u);
    size_t prefix_len = 8 - suffix_len;

    size_t out = 0;
    for (const char *p = name; p < stem_end && out < prefix_len; p++) {
        if (*p == '.' || *p == ' ') {
            continue;
        }
        sfn[out++] = sanitize_sfn_char(static_cast<uint8_t>(*p));
    }
    while (out < prefix_len) {
        sfn[out++] = '_';
    }
    for (size_t i = 0; i < suffix_len; i++) {
        sfn[prefix_len + i] = suffix[i];
    }

    memcpy(&sfn[8], ext, sizeof(ext));
}

status_t generate_unique_short_name_for_lfn(fat_fs *fat, uint32_t starting_dir_cluster,
                                            const char *name, char sfn[12]) {
    for (uint32_t ord = 1; ord < 1000000; ord++) {
        build_short_name_alias(name, ord, sfn);

        dir_entry existing;
        status_t err = fat_find_short_file_in_dir_with_offsets(fat, starting_dir_cluster, sfn,
                                                               &existing, nullptr, nullptr);
        if (err == ERR_NOT_FOUND) {
            // this short name is not taken, we can use it
            return NO_ERROR;
        } else if (err < 0) {
            return err;
        }
    }

    return ERR_ALREADY_EXISTS;
}

// walk one entry into the dir, starting at byte offset into the directory block iterator.
// both dbi and offset will be modified during the call.
// filles out the entry and returns a pointer into the passed in buffer in out_filename.
// NOTE: *must* pass at least a MAX_FILE_NAME_LEN byte char pointer in the filename_buffer slot.
status_t fat_find_next_entry(fat_fs *fat, file_block_iterator &dbi, uint32_t &offset, dir_entry *entry,
                             char filename_buffer[MAX_FILE_NAME_LEN], char **out_filename) {

    DEBUG_ASSERT(entry && filename_buffer && out_filename);

    // Note: offset is used as an ABSOLUTE directory offset (accounting for sector boundaries)
    // We track which sector we're in based on offset / bytes_per_sector

    // lfn parsing state: build UTF-8 string backwards from the end of filename_buffer.
    struct lfn_parse_state {
        size_t utf8_pos = 0;          // next byte position to write (decrements)
        uint8_t max_sequence = 0;     // highest sequence (the first LFN entry with 0x40)
        uint8_t last_sequence = 0xff; // last sequence seen (for validation)
        uint8_t checksum = 0;

        void reset() {
            utf8_pos = 0;
            max_sequence = 0;
            last_sequence = 0xff;
        }
    } lfn_state;

    for (;;) {
        if (LOCAL_TRACE >= 2) {
            LTRACEF("dir sector:\n");
            hexdump8_ex(dbi.get_bcache_ptr(0), fat->info().bytes_per_sector, 0);
        }

        // walk within a sector
        while (offset < fat->info().bytes_per_sector) {
            LTRACEF_LEVEL(2, "looking at offset %#x\n", offset);
            const uint8_t *ent = dbi.get_bcache_ptr(offset);
            if (ent[0] == 0) { // no more entries
                // we're completely done
                LTRACEF("completely done\n");
                return ERR_NOT_FOUND;
            } else if (ent[0] == 0xE5) { // deleted entry
                LTRACEF("deleted entry\n");
                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else if (ent[0x0B] == (uint8_t)fat_attribute::volume_id) {
                // skip volume ids
                LTRACEF("skipping volume id\n");
                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else if (ent[0x0B] == (uint8_t)fat_attribute::lfn) {
                // part of a LFN sequence
                uint8_t sequence = ent[0] & ~0x40;
                if (sequence == 0) {
                    // malformed LFN sequence, discard any accumulated state
                    LTRACEF("invalid LFN sequence 0\n");
                    lfn_state.reset();
                    offset += DIR_ENTRY_LENGTH;
                    continue;
                }
                // FAT stores LFN entries in reverse: highest sequence (with 0x40 flag)
                // first, then decreasing to sequence 1 immediately before the SFN.
                // We convert each UCS-2 code point to UTF-8 and write backwards
                // from the end of filename_buffer.
                if (ent[0] & 0x40) {
                    // end sequence (first LFN entry), start at end of buffer
                    lfn_state.utf8_pos = MAX_FILE_NAME_LEN;
                    lfn_state.max_sequence = sequence;
                    lfn_state.last_sequence = sequence;
                    lfn_state.checksum = ent[0x0d];
                    LTRACEF_LEVEL(2, "start of new LFN entry, sequence %u\n", sequence);
                } else {
                    if (lfn_state.last_sequence != sequence + 1) {
                        // our entry is out of sequence? drop it and start over
                        LTRACEF("ent out of sequence %u (last sequence %u)\n", sequence, lfn_state.last_sequence);
                        lfn_state.reset();
                        offset += DIR_ENTRY_LENGTH;
                        continue;
                    }
                    if (lfn_state.checksum != ent[0x0d]) {
                        // all of the long sequences need to match the checksum
                        LTRACEF("ent mismatches previous checksum\n");
                        lfn_state.reset();
                        offset += DIR_ENTRY_LENGTH;
                        continue;
                    }
                    lfn_state.last_sequence = sequence;
                }

                // extract unicode characters and convert to UTF-8, writing backwards.
                constexpr size_t table[] = {30, 28, 24, 22, 20, 18, 16, 14, 9, 7, 5, 3, 1};
                for (size_t i = 0; i < kFatLfnCharsPerEntry; i++) {
                    uint16_t c = fat_read16(ent, table[i]);
                    if (c == 0xffff || c == 0x0) {
                        continue;
                    }

                    // Convert one UCS2 char to up to 3 utf-8 bytes in a temporary buffer
                    char utf8_bytes[3];
                    size_t nbytes = ucs2_char_to_utf8(c, utf8_bytes);
                    if (lfn_state.utf8_pos < nbytes) {
                        LTRACEF("LFN too long for filename buffer\n");
                        lfn_state.reset();
                        break;
                    }

                    // Copy them into the output buffer in reverse order
                    for (size_t j = 0; j < nbytes; j++) {
                        filename_buffer[lfn_state.utf8_pos - nbytes + j] = utf8_bytes[j];
                    }
                    lfn_state.utf8_pos -= nbytes;
                }

                if (lfn_state.max_sequence == 0) {
                    offset += DIR_ENTRY_LENGTH;
                    continue;
                }

                // iterate one more entry, since we need to at least need to find the corresponding SFN
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else {
                // regular entry, extract the short file name
                char short_filename[8 + 1 + 3 + 1]; // max short name (8 . 3 NULL)
                size_t fname_pos = 0;

                // Ignore trailing spaces in filename and/or extension
                int fn_len = 8, ext_len = 3;
                for (int i = 7; i >= 0; i--) {
                    if (ent[i] == 0x20) {
                        fn_len--;
                    } else {
                        break;
                    }
                }
                for (size_t i = 10; i >= 8; i--) {
                    if (ent[i] == 0x20) {
                        ext_len--;
                    } else {
                        break;
                    }
                }

                for (int i = 0; i < fn_len; i++) {
                    short_filename[fname_pos++] = ent[i];
                }
                if (ext_len > 0) {
                    short_filename[fname_pos++] = '.';
                    for (int i = 0; i < ext_len; i++) {
                        short_filename[fname_pos++] = ent[8 + i];
                    }
                }
                short_filename[fname_pos++] = 0;
                DEBUG_ASSERT(fname_pos <= sizeof(short_filename));

                // now we have the SFN, see if we just got finished parsing a corresponding LFN
                // in the previous entries
                if (lfn_state.last_sequence == 1) {
                    uint8_t checksum = fat_lfn_sfn_checksum(ent);
                    if (checksum == lfn_state.checksum && lfn_state.utf8_pos < MAX_FILE_NAME_LEN) {
                        // move the backwards-built UTF-8 string to the start of the buffer
                        size_t utf8_len = MAX_FILE_NAME_LEN - lfn_state.utf8_pos;
                        memmove(filename_buffer, filename_buffer + lfn_state.utf8_pos, utf8_len);
                        filename_buffer[utf8_len] = '\0';
                        *out_filename = filename_buffer;
                    } else {
                        LTRACEF("LFN checksum mismatch, using SFN\n");
                        strlcpy(filename_buffer, short_filename, sizeof(short_filename));
                        *out_filename = filename_buffer;
                    }
                } else {
                    // copy the parsed short file name into the out buffer
                    strlcpy(filename_buffer, short_filename, sizeof(short_filename));
                    *out_filename = filename_buffer;
                }

                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;

                // fall through, we've found a file entry
            }

            LTRACEF("found filename '%s'\n", *out_filename);

            // fill out the passed in dir entry and exit
            uint16_t target_cluster = fat_read16(ent, 0x1a);
            entry->length = fat_read32(ent, 0x1c);
            entry->attributes = (fat_attribute)ent[0x0B];
            entry->start_cluster = target_cluster;
            return NO_ERROR;
        }

        DEBUG_ASSERT(offset <= fat->info().bytes_per_sector);

        // move to the next sector
        status_t err = dbi.next_sector();
        if (err < 0) {
            break;
        }
        // start over at offset 0 in the new sector
        offset = 0;
    }

    // we're out of entries
    return ERR_NOT_FOUND;
}

status_t fat_find_file_in_dir(fat_fs *fat, uint32_t starting_cluster, const char *name, dir_entry *entry, uint32_t *found_offset) {
    LTRACEF("start_cluster %u, name '%s', out entry %p\n", starting_cluster, name, entry);

    DEBUG_ASSERT(fat->lock.is_held());
    DEBUG_ASSERT(entry);

    // cache the length of the string we're matching against
    const size_t namelen = strlen(name);

    // kick start our directory sector iterator
    file_block_iterator dbi(fat, starting_cluster);
    status_t err = dbi.next_sectors(0);
    if (err < 0) {
        return err;
    }

    uint32_t offset = 0;
    uint32_t dir_offset_base = 0;
    for (;;) {
        char filename_buffer[MAX_FILE_NAME_LEN]; // max fat file name length
        char *filename;

        // Reset the sector increment count before calling fat_find_next_entry,
        // which may call next_sector one or more times.
        dbi.reset_sector_inc_count();

        // step forward one entry and see if we got something
        err = fat_find_next_entry(fat, dbi, offset, entry, filename_buffer, &filename);
        if (err < 0) {
            return err;
        }

        // Account for any sector increments that happened in fat_find_next_entry to keep an
        // absolute offset into the directory
        dir_offset_base += dbi.get_sector_inc_count() * fat->info().bytes_per_sector;

        const size_t filenamelen = strlen(filename);

        // see if we've matched an entry
        if (filenamelen == namelen && !strnicmp(name, filename, filenamelen)) {
            // we have, return with a good status
            if (found_offset) {
                *found_offset = dir_offset_base + offset;
            }
            return NO_ERROR;
        }
    }
}

status_t fat_find_file_in_dir_with_offsets(fat_fs *fat, uint32_t starting_cluster,
                                           const char *name, dir_entry *entry,
                                           uint32_t *entry_start_offset,
                                           uint32_t *entry_end_offset) {
    LTRACEF("start_cluster %u, name '%s', out entry %p\n", starting_cluster, name, entry);

    DEBUG_ASSERT(fat->lock.is_held());
    DEBUG_ASSERT(entry);

    const size_t namelen = strlen(name);

    file_block_iterator dbi(fat, starting_cluster);
    status_t err = dbi.next_sectors(0);
    if (err < 0) {
        return err;
    }

    uint32_t offset = 0;
    uint32_t dir_offset_base = 0;
    for (;;) {
        char filename_buffer[MAX_FILE_NAME_LEN];
        char *filename;
        uint32_t old_offset = dir_offset_base + offset;

        // Reset the sector increment count before calling fat_find_next_entry, which may call next_sector.
        dbi.reset_sector_inc_count();

        err = fat_find_next_entry(fat, dbi, offset, entry, filename_buffer, &filename);
        if (err < 0) {
            return err;
        }

        // Account for any sector increments that happened in fat_find_next_entry to keep an
        // absolute offset into the directory.
        dir_offset_base += dbi.get_sector_inc_count() * fat->info().bytes_per_sector;
        uint32_t new_offset = dir_offset_base + offset;

        const size_t filenamelen = strlen(filename);
        if (filenamelen == namelen && !strnicmp(name, filename, filenamelen)) {
            uint32_t record_start_offset = old_offset;

            // old_offset may include deleted entries that were skipped while walking to
            // this file record. Pick the first non-deleted/non-volume entry in the span.
            for (uint32_t probe_offset = old_offset; probe_offset < new_offset;
                 probe_offset += DIR_ENTRY_LENGTH) {
                file_block_iterator probe_iter(fat, starting_cluster);
                status_t probe_err = probe_iter.next_sectors(probe_offset / fat->info().bytes_per_sector);
                if (probe_err < 0) {
                    break;
                }

                const uint8_t *probe_ptr =
                    probe_iter.get_bcache_ptr(probe_offset % fat->info().bytes_per_sector);
                if (probe_ptr[0] == 0xE5 || probe_ptr[0] == 0x00 ||
                    probe_ptr[11] == static_cast<uint8_t>(fat_attribute::volume_id)) {
                    continue;
                }

                record_start_offset = probe_offset;
                break;
            }

            if (entry_start_offset) {
                *entry_start_offset = record_start_offset;
            }
            if (entry_end_offset) {
                *entry_end_offset = new_offset;
            }
            return NO_ERROR;
        }
    }
}

status_t fat_find_short_file_in_dir_with_offsets(fat_fs *fat, uint32_t starting_cluster,
                                                 const char short_name[11], dir_entry *entry,
                                                 uint32_t *entry_start_offset,
                                                 uint32_t *entry_end_offset) {
    LTRACEF("start_cluster %u, short_name '%.11s', out entry %p\n", starting_cluster, short_name,
            entry);

    DEBUG_ASSERT(fat->lock.is_held());
    DEBUG_ASSERT(entry);

    file_block_iterator dbi(fat, starting_cluster);
    status_t err = dbi.next_sectors(0);
    if (err < 0) {
        return err;
    }

    uint32_t dir_offset = 0;
    uint32_t sector_offset = 0;
    for (;;) {
        while (sector_offset < fat->info().bytes_per_sector) {
            const uint8_t *ent = dbi.get_bcache_ptr(sector_offset);
            if (ent[0] == 0) {
                return ERR_NOT_FOUND;
            }

            if (ent[0] == 0xE5 ||
                ent[0x0B] == (uint8_t)fat_attribute::volume_id ||
                ent[0x0B] == (uint8_t)fat_attribute::lfn) {
                dir_offset += DIR_ENTRY_LENGTH;
                sector_offset += DIR_ENTRY_LENGTH;
                continue;
            }

            if (!memcmp(ent, short_name, 11)) {
                uint16_t target_cluster = fat_read16(ent, 0x1a);
                entry->length = fat_read32(ent, 0x1c);
                entry->attributes = (fat_attribute)ent[0x0B];
                entry->start_cluster = target_cluster;

                if (entry_start_offset) {
                    *entry_start_offset = dir_offset;
                }
                if (entry_end_offset) {
                    *entry_end_offset = dir_offset + DIR_ENTRY_LENGTH;
                }
                return NO_ERROR;
            }

            dir_offset += DIR_ENTRY_LENGTH;
            sector_offset += DIR_ENTRY_LENGTH;
        }

        err = dbi.next_sector();
        if (err < 0) {
            return err;
        }
        sector_offset = 0;
    }
}

} // anonymous namespace

static status_t fat_dir_is_empty(fat_fs *fat, uint32_t starting_cluster) {
    DEBUG_ASSERT(fat->lock.is_held());

    if (starting_cluster < 2 || starting_cluster >= fat->info().total_clusters) {
        return ERR_BAD_STATE;
    }

    file_block_iterator dbi(fat, starting_cluster);
    status_t err = dbi.next_sectors(0);
    if (err < 0) {
        return err;
    }

    uint32_t offset = 0;
    for (;;) {
        char filename_buffer[MAX_FILE_NAME_LEN];
        char *filename;
        dir_entry entry;

        err = fat_find_next_entry(fat, dbi, offset, &entry, filename_buffer, &filename);
        if (err == ERR_NOT_FOUND) {
            return NO_ERROR;
        }
        if (err < 0) {
            return err;
        }

        if (!strcmp(filename, ".") || !strcmp(filename, "..")) {
            continue;
        }

        return ERR_NOT_ALLOWED;
    }
}

status_t fat_dir_walk(fat_fs *fat, const char *path, dir_entry *out_entry, dir_entry_location *loc) {
    LTRACEF("path %s\n", path);

    DEBUG_ASSERT(fat->lock.is_held());

    // routine to push the path element ahead one bump
    // will leave path pointing at the next element, and path_element_size
    // in characters for the next element (or 0 if finished).
    size_t path_element_size = 0;
    auto path_increment = [&path, &path_element_size]() {
        path += path_element_size;
        path_element_size = 0;

        // we're at the end of the string
        if (*path == 0) {
            return;
        }

        // push path past the next /
        while (*path == '/' && path != 0) {
            path++;
        }

        // count the size of the next element
        const char *ptr = path;
        while (*ptr != '/' && *ptr != 0) {
            ptr++;
            path_element_size++;
        }
    };

    // increment it once past leading / and establish an initial path_element_size
    path_increment();
    LTRACEF("first path '%s' path_element_size %zu\n", path, path_element_size);

    // set up the starting cluster to search
    uint32_t dir_start_cluster;
    if (fat->info().root_cluster) {
        dir_start_cluster = fat->info().root_cluster;
    } else {
        // fat 12/16 has a linear root dir, cluster 0 is a special case to fat_find_file_in_dir below
        dir_start_cluster = 0;
    }

    // output entry
    dir_entry entry{};

    // walk the directory structure
    for (;;) {
        char name_element[MAX_FILE_NAME_LEN];
        strlcpy(name_element, path, MIN(sizeof(name_element), path_element_size + 1));

        LTRACEF("searching for element %s\n", name_element);

        uint32_t found_offset = 0;
        auto status = fat_find_file_in_dir(fat, dir_start_cluster, name_element, &entry, &found_offset);
        if (status < 0) {
            return ERR_NOT_FOUND;
        }

        // we found something
        LTRACEF("found dir entry attributes %#hhx length %u start_cluster %u\n",
                (uint8_t)entry.attributes, entry.length, entry.start_cluster);

        // push the name element forward one notch so we can see if we're at the end (or iterate once again)
        path_increment();
        if (path_element_size > 0) {
            // did we find a directory on an inner node of the path? we can continue iterating
            if (entry.attributes == fat_attribute::directory) {
                dir_start_cluster = entry.start_cluster;
            } else {
                LTRACEF("found a non dir at a non terminal path entry, exit\n");
                return ERR_NOT_FOUND;
            }
        } else {
            // we got a hit at the terminal entry of the path, pass it out to the caller as a success
            if (out_entry) {
                *out_entry = entry;
            }
            if (loc) {
                loc->starting_dir_cluster = dir_start_cluster;
                loc->dir_offset = found_offset;
            }
            return NO_ERROR;
        }
    }
}

// splits a path into the part of it leading up to the last element and the last element
// if the leading part is zero length, return a single "/" element
// will modify string passed in
void split_path(char *path, const char **leading_path, const char **last_element) {
    char *last_slash = strrchr(path, '/');
    if (last_slash) {
        *last_slash = 0;
        if (path[0] != 0) {
            *leading_path = path;
        } else {
            *leading_path = "/";
        }
        *last_element = last_slash + 1;
    } else {
        *leading_path = "/";
        *last_element = path;
    }
}

// construct a short file name from the incoming name
// the sfn is padded out with spaces the same way a real FAT entry is
status_t name_to_short_file_name(char sfn[8 + 3 + 1], const char *name) {
    // zero length inputs don't fly
    if (name[0] == 0) {
        return ERR_INVALID_ARGS;
    }

    // start off with a spaced out sfn
    memset(sfn, ' ', 8 + 3);
    sfn[8 + 3] = 0;

    size_t input_pos = 0;
    size_t output_pos = 0;

    // pick out the 8 entry part
    for (auto i = 0; i < 8; i++) {
        char c = name[input_pos];
        if (c == 0) {
            break;
        } else if (c == '.') {
            output_pos = 8;
            break;
        } else {
            sfn[output_pos++] = toupper(c);
            input_pos++;
        }
    }

    // at this point input pos had better be looking at a . or a null
    if (name[input_pos] == 0) {
        return NO_ERROR;
    }
    if (name[input_pos] != '.') {
        return ERR_INVALID_ARGS;
    }
    input_pos++;

    for (auto i = 0; i < 3; i++) {
        char c = name[input_pos];
        if (c == 0) {
            break;
        } else if (c == '.') {
            // can only see '.' once
            return ERR_INVALID_ARGS;
        } else {
            sfn[output_pos++] = toupper(c);
            input_pos++;
        }
    }

    // at this point we should be looking at the end of the input string
    if (name[input_pos] != 0) {
        return ERR_INVALID_ARGS;
    }

    return NO_ERROR;
}

namespace {

void fill_short_dirent(uint8_t *ent, const char short_name[11], fat_attribute attr,
                       uint32_t starting_cluster, uint32_t size) {
    memcpy(&ent[0], short_name, 11);              // name
    ent[11] = (uint8_t)attr;                      // attribute
    ent[12] = 0;                                  // reserved
    ent[13] = 0;                                  // creation time tenth of second
    fat_write16(ent, 14, 0);                      // creation time seconds / 2
    fat_write16(ent, 16, 0);                      // creation date
    fat_write16(ent, 18, 0);                      // last accessed date
    fat_write16(ent, 20, starting_cluster >> 16); // fat cluster high
    fat_write16(ent, 22, 0);                      // modification time
    fat_write16(ent, 24, 0);                      // modification date
    fat_write16(ent, 26, starting_cluster);       // fat cluster low
    fat_write32(ent, 28, size);                   // file size
}

// given a dir entry location, open the corresponding sector and pass back a open pointer
// into the block cache.
// this code encapsulates the logic that takes into account that cluster 0 is magic in
// fat 12 and fat 16 for the root dir.
bcache_block_ref open_dirent_block(fat_fs *fat, const dir_entry_location &loc) {
    LTRACEF("fat %p, loc %u:%u\n", fat, loc.starting_dir_cluster, loc.dir_offset);

    uint32_t cluster = loc.starting_dir_cluster;
    uint32_t offset = loc.dir_offset;
    uint32_t sector;

    if (cluster == 0) {
        DEBUG_ASSERT(fat->info().fat_bits == 12 || fat->info().fat_bits == 16);
        // Special case on FAT12/16 to represent the root dir.
        DEBUG_ASSERT(offset < fat->info().root_entries * DIR_ENTRY_LENGTH);
        sector = fat->info().root_start_sector + offset / fat->info().bytes_per_sector;
    } else {
        // Walk the cluster chain if the offset exceeds the current cluster.
        const uint32_t bytes_per_cluster = fat->info().bytes_per_sector * fat->info().sectors_per_cluster;
        while (offset >= bytes_per_cluster) {
            if (is_eof_cluster(cluster)) {
                return bcache_block_ref(fat->bcache());
            }
            cluster = fat_next_cluster_in_chain(fat, cluster);
            if (is_eof_cluster(cluster)) {
                return bcache_block_ref(fat->bcache());
            }
            offset -= bytes_per_cluster;
        }
        uint32_t cluster_sector = fat_sector_for_cluster(fat, cluster);
        if (cluster_sector == 0xffffffff) {
            return bcache_block_ref(fat->bcache());
        }
        sector = cluster_sector + offset / fat->info().bytes_per_sector;
    }

    bcache_block_ref bref(fat->bcache());
    bref.get_block(sector);

    return bref;
}

status_t resolve_parent_cluster_and_last_element(fat_fs *fat, const char *path,
                                                 char local_path[FS_MAX_FILE_LEN + 1],
                                                 uint32_t *parent_cluster,
                                                 const char **last_element) {
    strlcpy(local_path, path, FS_MAX_FILE_LEN + 1);

    const char *leading_path;
    split_path(local_path, &leading_path, last_element);

    if (!(*last_element) || (*last_element)[0] == 0) {
        return ERR_INVALID_ARGS;
    }

    if (strcmp(leading_path, "/") == 0) {
        *parent_cluster = fat->info().root_cluster ? fat->info().root_cluster : 0;
        return NO_ERROR;
    }

    dir_entry parent_entry;
    status_t err = fat_dir_walk(fat, leading_path, &parent_entry, nullptr);
    if (err < 0) {
        return err;
    }
    if (parent_entry.attributes != fat_attribute::directory) {
        return ERR_BAD_PATH;
    }

    *parent_cluster = parent_entry.start_cluster;
    if (*parent_cluster < 2 || *parent_cluster >= fat->info().total_clusters) {
        return ERR_BAD_STATE;
    }

    return NO_ERROR;
}

status_t find_entry_in_parent_for_unlink(fat_fs *fat, uint32_t parent_cluster,
                                         const char *name, dir_entry *entry,
                                         uint32_t *entry_start_offset,
                                         uint32_t *entry_end_offset) {
    char short_name[8 + 3 + 1];
    status_t err = name_to_short_file_name(short_name, name);
    if (err == NO_ERROR) {
        return fat_find_short_file_in_dir_with_offsets(fat, parent_cluster, short_name, entry,
                                                       entry_start_offset, entry_end_offset);
    }

    return fat_find_file_in_dir_with_offsets(fat, parent_cluster, name, entry,
                                             entry_start_offset, entry_end_offset);
}

status_t check_entry_not_busy(fat_fs *fat, uint32_t parent_cluster, uint32_t entry_end_offset) {
    if (entry_end_offset < DIR_ENTRY_LENGTH) {
        return ERR_BAD_STATE;
    }

    dir_entry_location sfn_loc = {
        .starting_dir_cluster = parent_cluster,
        .dir_offset = entry_end_offset - DIR_ENTRY_LENGTH,
    };
    dir_entry_location walk_loc = {
        .starting_dir_cluster = parent_cluster,
        .dir_offset = entry_end_offset,
    };

    if (fat->lookup_file(sfn_loc) || fat->lookup_file(walk_loc)) {
        return ERR_BUSY;
    }

    return NO_ERROR;
}

status_t mark_entry_record_deleted(fat_fs *fat, uint32_t parent_cluster,
                                   uint32_t entry_start_offset,
                                   uint32_t entry_end_offset) {
    LTRACEF("cluster=%u, start=%u, end=%u\n",
            parent_cluster, entry_start_offset, entry_end_offset);

    if (entry_start_offset >= entry_end_offset ||
        (entry_start_offset % DIR_ENTRY_LENGTH) != 0 ||
        (entry_end_offset % DIR_ENTRY_LENGTH) != 0) {
        return ERR_BAD_STATE;
    }

    for (uint32_t offset = entry_start_offset; offset < entry_end_offset; offset += DIR_ENTRY_LENGTH) {
        LTRACEF("Deleting entry at offset %u\n", offset);
        dir_entry_location loc = {
            .starting_dir_cluster = parent_cluster,
            .dir_offset = offset,
        };
        bcache_block_ref bref = open_dirent_block(fat, loc);
        if (!bref.is_valid()) {
            LTRACEF("ERROR: open_dirent_block failed!\n");
            return ERR_IO;
        }

        uint8_t *ent = (uint8_t *)bref.ptr();
        ent += loc.dir_offset % fat->info().bytes_per_sector;
        LTRACEF_LEVEL(2, "Original byte: 0x%02x\n", ent[0]);
        ent[0] = 0xE5;
        LTRACEF_LEVEL(2, "After marking: 0x%02x\n", ent[0]);
        bref.mark_dirty();
    }

    return NO_ERROR;
}

} // namespace

// static
status_t fat_dir::mkdir(fscookie *cookie, const char *path) {
    auto *fat = (fat_fs *)cookie;

    LTRACEF("cookie %p path '%s'\n", cookie, path);

    AutoLock guard(fat->lock);

    char local_path[FS_MAX_FILE_LEN + 1];
    strlcpy(local_path, path, sizeof(local_path));

    const char *leading_path;
    const char *last_element;
    split_path(local_path, &leading_path, &last_element);

    if (!last_element || last_element[0] == 0) {
        return ERR_INVALID_ARGS;
    }

    uint32_t parent_cluster;
    uint32_t parent_cluster_for_dotdot;
    if (strcmp(leading_path, "/") == 0) {
        parent_cluster = fat->info().root_cluster ? fat->info().root_cluster : 0;
        parent_cluster_for_dotdot = 0;
    } else {
        dir_entry parent_entry;
        status_t err = fat_dir_walk(fat, leading_path, &parent_entry, nullptr);
        if (err < 0) {
            return err;
        }
        if (parent_entry.attributes != fat_attribute::directory) {
            return ERR_BAD_PATH;
        }
        parent_cluster = parent_entry.start_cluster;
        parent_cluster_for_dotdot = parent_cluster;
        if (parent_cluster < 2 || parent_cluster >= fat->info().total_clusters) {
            return ERR_BAD_STATE;
        }
    }

    uint32_t first_cluster = 0;
    uint32_t last_cluster = 0;
    status_t err = fat_allocate_cluster_chain(fat, 0, 1, &first_cluster, &last_cluster, true);
    if (err != NO_ERROR) {
        return err;
    }

    dir_entry_location loc;
    err = fat_dir_allocate(fat, path, fat_attribute::directory, first_cluster, 0, &loc);
    if (err != NO_ERROR) {
        fat_free_cluster_chain(fat, first_cluster);
        return err;
    }

    bcache_block_ref bref(fat->bcache());
    uint32_t sector = fat_sector_for_cluster(fat, first_cluster);
    if (sector == 0xffffffff) {
        return ERR_INVALID_ARGS;
    }
    err = bref.get_block(sector);
    if (err < 0) {
        return err;
    }

    char dot_name[11];
    memset(dot_name, ' ', sizeof(dot_name));
    dot_name[0] = '.';

    char dotdot_name[11];
    memset(dotdot_name, ' ', sizeof(dotdot_name));
    dotdot_name[0] = '.';
    dotdot_name[1] = '.';

    uint8_t *block = (uint8_t *)bref.ptr();
    fill_short_dirent(block + 0 * DIR_ENTRY_LENGTH, dot_name, fat_attribute::directory,
                      first_cluster, 0);
    fill_short_dirent(block + 1 * DIR_ENTRY_LENGTH, dotdot_name, fat_attribute::directory,
                      parent_cluster_for_dotdot, 0);
    bref.mark_dirty();

    bcache_flush(fat->bcache());

    return NO_ERROR;
}

// static
status_t fat_dir::remove(fscookie *cookie, const char *path) {
    auto *fat = (fat_fs *)cookie;

    LTRACEF("cookie %p path '%s'\n", cookie, path);

    AutoLock guard(fat->lock);

    char local_path[FS_MAX_FILE_LEN + 1];
    uint32_t parent_cluster;
    const char *last_element;
    status_t err = resolve_parent_cluster_and_last_element(fat, path, local_path,
                                                           &parent_cluster, &last_element);
    if (err < 0) {
        return err;
    }

    dir_entry entry;
    uint32_t entry_start_offset = 0;
    uint32_t entry_end_offset = 0;
    err = find_entry_in_parent_for_unlink(fat, parent_cluster, last_element, &entry,
                                          &entry_start_offset, &entry_end_offset);
    if (err < 0) {
        return err;
    }

    LTRACEF("found entry: attributes %#hhx length %u start_cluster %u\n",
            (uint8_t)entry.attributes, entry.length, entry.start_cluster);
    LTRACEF("entry offsets: start %u end %u\n", entry_start_offset, entry_end_offset);

    if (entry.attributes == fat_attribute::directory) {
        return ERR_NOT_FILE;
    }

    err = check_entry_not_busy(fat, parent_cluster, entry_end_offset);
    if (err < 0) {
        return err;
    }

    if (entry.start_cluster != 0) {
        if (entry.start_cluster < 2 || entry.start_cluster >= fat->info().total_clusters) {
            return ERR_BAD_STATE;
        }

        err = fat_free_cluster_chain(fat, entry.start_cluster);
        if (err != NO_ERROR) {
            return err;
        }
    }

    // Mark all dir entries in this file's record (LFN entries plus SFN entry) as deleted.
    LTRACEF("Marking entries deleted: start=%u, end=%u\n", entry_start_offset, entry_end_offset);
    err = mark_entry_record_deleted(fat, parent_cluster, entry_start_offset, entry_end_offset);
    if (err < 0) {
        LTRACEF("mark_entry_record_deleted failed: %d\n", err);
        return err;
    }

    LTRACEF("Flushing bcache...\n");
    bcache_flush(fat->bcache());
    LTRACEF("Remove complete\n");

    return NO_ERROR;
}

// static
status_t fat_dir::rmdir(fscookie *cookie, const char *path) {
    auto *fat = (fat_fs *)cookie;

    LTRACEF("cookie %p path '%s'\n", cookie, path);

    AutoLock guard(fat->lock);

    char local_path[FS_MAX_FILE_LEN + 1];
    uint32_t parent_cluster;
    const char *last_element;
    status_t err = resolve_parent_cluster_and_last_element(fat, path, local_path,
                                                           &parent_cluster, &last_element);
    if (err < 0) {
        return err;
    }

    dir_entry entry;
    uint32_t entry_start_offset = 0;
    uint32_t entry_end_offset = 0;
    err = find_entry_in_parent_for_unlink(fat, parent_cluster, last_element, &entry,
                                          &entry_start_offset, &entry_end_offset);
    if (err < 0) {
        return err;
    }

    LTRACEF("found entry: attributes %#hhx length %u start_cluster %u\n",
            (uint8_t)entry.attributes, entry.length, entry.start_cluster);
    LTRACEF("entry offsets: start %u end %u\n", entry_start_offset, entry_end_offset);

    if (entry.attributes != fat_attribute::directory) {
        return ERR_NOT_DIR;
    }

    if (entry.start_cluster < 2 || entry.start_cluster >= fat->info().total_clusters) {
        return ERR_BAD_STATE;
    }

    err = check_entry_not_busy(fat, parent_cluster, entry_end_offset);
    if (err < 0) {
        return err;
    }

    err = fat_dir_is_empty(fat, entry.start_cluster);
    if (err < 0) {
        return err;
    }

    err = fat_free_cluster_chain(fat, entry.start_cluster);
    if (err != NO_ERROR) {
        return err;
    }

    // Mark all dir entries in this directory record (LFN entries plus SFN entry) as deleted.
    LTRACEF("Marking entries deleted: start=%u, end=%u\n", entry_start_offset, entry_end_offset);
    err = mark_entry_record_deleted(fat, parent_cluster, entry_start_offset, entry_end_offset);
    if (err < 0) {
        return err;
    }

    bcache_flush(fat->bcache());

    return NO_ERROR;
}

status_t fat_dir_allocate(fat_fs *fat, const char *path, const fat_attribute attr, const uint32_t starting_cluster, const uint32_t size, dir_entry_location *loc) {
    LTRACEF("path %s\n", path);

    DEBUG_ASSERT(fat->lock.is_held());

    // trim the last segment off the path, splitting into stuff leading up to the last segment and the last segment
    char local_path[FS_MAX_FILE_LEN + 1];
    strlcpy(local_path, path, FS_MAX_FILE_LEN);

    const char *leading_path;
    const char *last_element;
    split_path(local_path, &leading_path, &last_element);

    DEBUG_ASSERT(leading_path && last_element);

    LTRACEF("path is now split into %s and %s\n", leading_path, last_element);

    // find the starting directory cluster of the container directory
    // 0 may mean root dir on fat12/16
    uint32_t starting_dir_cluster;
    if (strcmp(leading_path, "/") == 0) {
        // root dir is a special case since we know where to start
        if (fat->info().root_cluster) {
            starting_dir_cluster = fat->info().root_cluster;
        } else {
            // fat 12/16 has a linear root dir, cluster 0 is a special case to fat_find_file_in_dir below
            starting_dir_cluster = 0;
        }
    } else {
        // walk to find the containing directory
        dir_entry entry;
        dir_entry_location dir_loc;
        status_t err = fat_dir_walk(fat, local_path, &entry, &dir_loc);
        if (err < 0) {
            return err;
        }

        // verify it's a directory
        if (entry.attributes != fat_attribute::directory) {
            return ERR_BAD_PATH;
        }

        LTRACEF("found containing dir at %u:%u: starting cluster %u\n", dir_loc.starting_dir_cluster, dir_loc.dir_offset, entry.start_cluster);

        starting_dir_cluster = entry.start_cluster;
        if (starting_dir_cluster < 2 || starting_dir_cluster >= fat->info().total_clusters) {
            TRACEF("directory entry contains out of bounds cluster %u\n", starting_dir_cluster);
            return ERR_BAD_STATE;
        }
    }

    LTRACEF("starting dir cluster of parent dir %u\n", starting_dir_cluster);

    // verify the file doesn't already exist
    dir_entry entry;
    status_t err = fat_find_file_in_dir(fat, starting_dir_cluster, last_element, &entry, nullptr);
    if (err >= 0) {
        // we found it, cant create a new file in its place
        return ERR_ALREADY_EXISTS;
    }

    char sfn[8 + 3 + 1];
    uint16_t *lfn_ucs2 = nullptr;
    auto lfn_cleanup = lk::make_auto_call([&]() { free(lfn_ucs2); });
    size_t lfn_ucs2_len = 0;
    bool needs_lfn = false;

    err = name_to_short_file_name(sfn, last_element);
    if (err < 0) {
        lfn_ucs2 = static_cast<uint16_t *>(malloc(sizeof(uint16_t) * kFatMaxLfnChars));
        if (!lfn_ucs2) {
            return ERR_NO_MEMORY;
        }

        status_t lfn_err = fat_utf8_to_ucs2(last_element, lfn_ucs2, kFatMaxLfnChars, &lfn_ucs2_len);
        if (lfn_err < 0) {
            return lfn_err;
        }

        if (lfn_ucs2_len == 0) {
            return ERR_INVALID_ARGS;
        }

        err = generate_unique_short_name_for_lfn(fat, starting_dir_cluster, last_element, sfn);
        if (err < 0) {
            return err;
        }
        needs_lfn = true;
    }

    LTRACEF("short file name '%s'\n", sfn);

    size_t lfn_entry_count = needs_lfn ? ((lfn_ucs2_len + kFatLfnCharsPerEntry - 1) / kFatLfnCharsPerEntry) : 0;
    size_t total_entry_count = lfn_entry_count + 1;

    // now we have a starting cluster for the containing directory and proof that it doesn't already exist.
    // start walking to find enough contiguous free slots for [LFN entries..., SFN entry].
    uint32_t run_start_offset = 0;
    uint32_t run_len = 0;
    bool found_run = false;

    for (;;) {
        file_block_iterator dbi(fat, starting_dir_cluster);
        err = dbi.next_sectors(0);
        if (err < 0) {
            return err;
        }

        uint32_t dir_offset = 0;
        uint32_t sector_offset = 0;
        run_len = 0;
        found_run = false;

        for (;;) {
            if (LOCAL_TRACE >= 2) {
                LTRACEF("dir sector:\n");
                hexdump8_ex(dbi.get_bcache_ptr(0), fat->info().bytes_per_sector, 0);
            }

            while (sector_offset < fat->info().bytes_per_sector) {
                LTRACEF_LEVEL(2, "looking at offset %#x\n", sector_offset);
                uint8_t *ent = dbi.get_bcache_ptr(sector_offset);

                if (ent[0] == 0xe5 || ent[0] == 0) {
                    if (run_len == 0) {
                        run_start_offset = dir_offset;
                    }
                    run_len++;
                    if (run_len >= total_entry_count) {
                        found_run = true;
                        break;
                    }
                } else {
                    run_len = 0;
                }

                dir_offset += DIR_ENTRY_LENGTH;
                sector_offset += DIR_ENTRY_LENGTH;
            }

            if (found_run) {
                break;
            }

            err = dbi.next_sector();
            if (err < 0) {
                if (err == ERR_OUT_OF_RANGE) {
                    break;
                }
                return err;
            }
            sector_offset = 0;
        }

        if (found_run) {
            break;
        }

        // Need more room.
        if (starting_dir_cluster == 0) {
            // Root directory on FAT12/16 is fixed size and cannot grow.
            return ERR_NO_MEMORY;
        }

        uint32_t last_cluster = fat_find_last_cluster_in_chain(fat, starting_dir_cluster);
        uint32_t new_cluster;
        uint32_t last_allocated;
        err = fat_allocate_cluster_chain(fat, last_cluster, 1, &new_cluster, &last_allocated, true);
        if (err != NO_ERROR) {
            return err;
        }
    }

    uint8_t checksum = fat_lfn_sfn_checksum(reinterpret_cast<const uint8_t *>(sfn));
    for (size_t i = 0; i < total_entry_count; i++) {
        dir_entry_location entry_loc = {
            .starting_dir_cluster = starting_dir_cluster,
            .dir_offset = run_start_offset + static_cast<uint32_t>(i * DIR_ENTRY_LENGTH),
        };
        bcache_block_ref bref = open_dirent_block(fat, entry_loc);
        if (!bref.is_valid()) {
            return ERR_IO;
        }

        uint8_t *ent = static_cast<uint8_t *>(bref.ptr());
        ent += entry_loc.dir_offset % fat->info().bytes_per_sector;

        if (i < lfn_entry_count) {
            size_t reverse = lfn_entry_count - i;
            fill_lfn_dirent(ent, lfn_ucs2, lfn_ucs2_len,
                            static_cast<uint8_t>(reverse),
                            (reverse == lfn_entry_count), checksum);
        } else {
            fill_short_dirent(ent, sfn, attr, starting_cluster, size);
        }
        bref.mark_dirty();
    }

    bcache_flush(fat->bcache());

    if (loc) {
        loc->starting_dir_cluster = starting_dir_cluster;
        loc->dir_offset = run_start_offset + static_cast<uint32_t>(lfn_entry_count * DIR_ENTRY_LENGTH);
    }

    return NO_ERROR;
}

// update the starting cluster and/or size pointer in a directory entry
status_t fat_dir_update_entry(fat_fs *fat, const dir_entry_location &loc, uint32_t starting_cluster, uint32_t size) {
    LTRACEF("fat %p, loc %u:%u, cluster %u, size %u\n", fat, loc.starting_dir_cluster, loc.dir_offset, starting_cluster, size);

    bcache_block_ref bref = open_dirent_block(fat, loc);

    DEBUG_ASSERT(bref.is_valid());

    uint8_t *ent = (uint8_t *)bref.ptr();
    ent += loc.dir_offset % fat->info().bytes_per_sector;

    fat_write32(ent, 28, size);                   // file size
    fat_write16(ent, 20, starting_cluster >> 16); // fat cluster high
    fat_write16(ent, 26, starting_cluster);       // fat cluster low

    bref.mark_dirty();

    return NO_ERROR;
}

status_t fat_dir::opendir_priv(const dir_entry &entry, const dir_entry_location &loc, fat_dir_cookie **out_cookie) {
    // fill in our file info based on the entry
    start_cluster_ = entry.start_cluster;
    length_ = 0; // dirs all have 0 length entry
    dir_loc_ = loc;
    inc_ref();

    // create a dir cookie
    auto dir_cookie = new fat_dir_cookie;
    dir_cookie->dir = this;
    dir_cookie->index = 0;

    // add it to the dir object
    list_add_tail(&cookies_, &dir_cookie->node);

    *out_cookie = dir_cookie;

    return NO_ERROR;
}

status_t fat_dir::opendir(fscookie *cookie, const char *name, dircookie **dcookie) {
    auto fat = (fat_fs *)cookie;

    LTRACEF("cookie %p name '%s' dircookie %p\n", cookie, name, dcookie);

    AutoLock guard(fat->lock);

    dir_entry entry;
    dir_entry_location loc;

    // special case for /
    if (name[0] == 0 || !strcmp(name, "/")) {
        entry.attributes = fat_attribute::directory;
        entry.length = 0;
        if (fat->info().fat_bits == 32) {
            entry.start_cluster = fat->info().root_cluster;
        } else {
            entry.start_cluster = 0;
        }

        // special case for the root dir
        // 0:0 is not sufficient, since we could actually find a file in the root dir
        // on a fat 12/16 volume (magic cluster 0) at offset 0. cluster 1 is never used
        // so mark root dir as 1:0
        loc.starting_dir_cluster = 1;
        loc.dir_offset = 0;
    } else {
        status_t err = fat_dir_walk(fat, name, &entry, &loc);
        if (err != NO_ERROR) {
            return err;
        }
    }

    // if we walked and found a proper directory, it's a hit
    if (entry.attributes == fat_attribute::directory) {
        fat_dir *dir;

        // see if this dir is already present in the fs list
        fat_file *file = fat->lookup_file(loc);
        if (file) {
            // XXX replace with hand rolled RTTI
            dir = reinterpret_cast<fat_dir *>(file);
        } else {
            dir = new fat_dir(fat);
        }
        DEBUG_ASSERT(dir);

        fat_dir_cookie *dir_cookie;

        status_t err = dir->opendir_priv(entry, loc, &dir_cookie);
        if (err < 0) {
            // weird state, should we dec the ref?
            PANIC_UNIMPLEMENTED;
            return err;
        }
        DEBUG_ASSERT(dir_cookie);

        *dcookie = (dircookie *)dir_cookie;
        return NO_ERROR;
    } else {
        return ERR_NOT_FILE;
    }

    return ERR_NOT_IMPLEMENTED;
};

status_t fat_dir::readdir_priv(fat_dir_cookie *cookie, struct dirent *ent) {
    LTRACEF("dircookie %p ent %p, current index %u\n", cookie, ent, cookie->index);

    if (!ent) {
        return ERR_INVALID_ARGS;
    }

    // make sure the cookie makes sense
    DEBUG_ASSERT((cookie->index % DIR_ENTRY_LENGTH) == 0);

    char filename_buffer[MAX_FILE_NAME_LEN];
    char *filename;
    dir_entry entry;

    {
        AutoLock guard(fs_->lock);

        // kick start our directory sector iterator
        LTRACEF("start cluster %u\n", start_cluster_);
        file_block_iterator dbi(fs_, start_cluster_);

        // move it forward to our index point
        // also loads the buffer
        status_t err = dbi.next_sectors(cookie->index / fs_->info().bytes_per_sector);
        if (err < 0) {
            return err;
        }

        // reset how many sectors the dbi has pushed forward so we can account properly for index shifts later
        dbi.reset_sector_inc_count();

        // pass the index in units of sector offset
        uint32_t offset = cookie->index % fs_->info().bytes_per_sector;
        err = fat_find_next_entry(fs_, dbi, offset, &entry, filename_buffer, &filename);
        if (err < 0) {
            return err;
        }

        // bump the index forward by extracting how much the sector iterator pushed things forward
        uint32_t index_inc = offset - (cookie->index % fs_->info().bytes_per_sector);
        index_inc += dbi.get_sector_inc_count() * fs_->info().bytes_per_sector;
        LTRACEF("calculated index increment %u (old index %u, offset %u, sector_inc_count %u)\n",
                index_inc, cookie->index, offset, dbi.get_sector_inc_count());
        cookie->index += index_inc;
    }

    // copy the info into the fs layer's entry
    strlcpy(ent->name, filename, MIN(sizeof(ent->name), MAX_FILE_NAME_LEN));

    return NO_ERROR;
}

status_t fat_dir::readdir(dircookie *dcookie, struct dirent *ent) {
    auto cookie = (fat_dir_cookie *)dcookie;
    auto dir = cookie->dir;

    return dir->readdir_priv(cookie, ent);
}

status_t fat_dir::closedir_priv(fat_dir_cookie *cookie, bool *last_ref) {
    LTRACEF("dircookie %p\n", cookie);

    AutoLock guard(fs_->lock);

    // remove the dircookie from the list
    DEBUG_ASSERT(list_in_list(&cookie->node));
    list_delete(&cookie->node);

    // delete it
    delete cookie;

    // drop a ref to the dir
    *last_ref = dec_ref();
    if (*last_ref) {
        DEBUG_ASSERT(list_is_empty(&cookies_));
    }

    return NO_ERROR;
}

status_t fat_dir::closedir(dircookie *dcookie) {
    auto cookie = (fat_dir_cookie *)dcookie;
    auto dir = cookie->dir;

    bool last_ref;
    status_t err = dir->closedir_priv(cookie, &last_ref);
    if (err < 0) {
        return err;
    }

    if (last_ref) {
        LTRACEF("last ref, deleting %p (%u:%u)\n", dir, dir->dir_loc().starting_dir_cluster, dir->dir_loc().dir_offset);
        delete dir;
    }

    return NO_ERROR;
}
