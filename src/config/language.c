/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include <stdlib.h>

u16 fnv_16_str(const char *str);
static char strings[4096];
#define MAX_STRINGS 256
#define MAX_LINE 200

struct str_map {
    u16 hash;
    u16 pos;
} lookupmap[MAX_STRINGS];

const char *_tr(const char *str)
{
    int i;
    if (lookupmap[0].pos == 0xffff) {
        return str;
    }
    u16 hash = fnv_16_str(str);
    for(i = 0; i < MAX_STRINGS; i++) {
        if(lookupmap[i].pos == 0xffff)
            return str;
        if(lookupmap[i].hash == hash)
            return strings + lookupmap[i].pos;
    }
    return str;
}

void CONFIG_ReadLang(const char *file)
{
    lookupmap[0].pos = 0xffff;
    if (! file)
        return;
    FILE *fh = fopen(file, "r");
    struct str_map *lookup = lookupmap;
    unsigned pos = 0;
    char line[MAX_LINE];
    if (! fh) {
        printf("Couldn't open language file: %s\n", file);
        return;
    }
    while (fgets(line, sizeof(line), fh) != NULL) {
        u16 hash;
        if(line[0] == ':') {
            unsigned i;
            unsigned len = strlen(line);
            for(i = 0; i < len; i++) {
                if(line[i] == 0x0a) {
                    line[i] = 0;
                    break;
                }
                if(line[i] == 0x0d)
                    line[i] = 0x0a;
            }
            hash = fnv_16_str(line + 1);
            if (fgets(line, sizeof(line), fh) == NULL) {
                break;
            }
            line[MAX_LINE-1] = 0;
            len = strlen(line);
            if (len > 2 && line[len-2] == '\n') {
                line[len-2] = 0;
                len--;
            }
            for(i = 0; i < len; i++) {
                if(line[i] == 0x0d)
                    line[i] = 0x0a;
            }
            if (pos + len > sizeof(strings)) {
                printf("Out of space processing %s\n", line);
                break;
            }
            lookup->hash = hash;
            lookup->pos = pos;
            lookup++;
            strncpy(strings + pos, line, sizeof(line));
            pos += len + 1;
        }
    }
    lookup->pos = 0xFFFF;
    fclose(fh);
}

/* The following section containing the FNV hash is in the public domain
 *
 * hash_32 - 32 bit Fowler/Noll/Vo hash code
 *
 * @(#) $Revision: 5.1 $
 ***
 * Fowler/Noll/Vo hash
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * To use the recommended 32 bit FNV-1 hash, pass FNV1_32_INIT as the
 * Fnv32_t hashval argument to fnv_32_buf() or fnv_32_str().
 ***
 * Please do not copyright this code.  This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * By:
 *	chongo <Landon Curt Noll> /\oo/\
 *      http://www.isthe.com/chongo/
 *
 */

/*
 * 32 bit magic FNV-0 and FNV-1 prime
 */
#define FNV_32_PRIME ((u32)0x01000193)
#define FNV1_32_INIT ((u32)0x811c9dc5)
#define MASK_16 (((u32)1<<16)-1) /* i.e., (u_int32_t)0xffff */

/*
 * fnv_16_str - perform a 32 bit Fowler/Noll/Vo hash on a string then fold to 16bit
 *
 * input:
 *	str	- string to hash
 *
 * returns:
 *	16 bit hash as a static hash type
 *
 */
u16 fnv_16_str(const char *str)
{
    unsigned char *s = (unsigned char *)str;	/* unsigned string */
    u32 hval = FNV1_32_INIT;
    /*
     * FNV-1 hash each octet in the buffer
     */
    while (*s) {

	/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
	hval *= FNV_32_PRIME;
#else
	hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif

	/* xor the bottom with the current octet */
	hval ^= (u32)*s++;
    }

    /* fold to 16bits (don't do this if you want 32bits */
    hval = (hval>>16) ^ (hval & MASK_16);
    /* return our new hash value */
    return hval;
}
