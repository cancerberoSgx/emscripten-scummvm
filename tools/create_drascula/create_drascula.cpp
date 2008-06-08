/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 * This is a utility for storing all the hardcoded data of Drascula in a separate
 * data file, used by the game engine
 */
 
 // HACK to allow building with the SDL backend on MinGW
// see bug #1800764 "TOOLS: MinGW tools building broken"
#ifdef main
#undef main
#endif // main

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "create_drascula.h"
#include "staticdata.h"

#define DRASCULA_DAT_VER 1	// 1 byte

static void writeByte(FILE *fp, uint8 b) {
	fwrite(&b, 1, 1, fp);
}

static void writeUint16BE(FILE *fp, uint16 value) {
	writeByte(fp, (uint8)(value >> 8));
	writeByte(fp, (uint8)(value & 0xFF));
}

int main(int argc, char *argv[]) {
	FILE* outFile;
	int i, lang;
	int len, pad;
	uint8 padBuf[DATAALIGNMENT];

	outFile = fopen("drascula.dat", "wb");

	// Write header
	fwrite("DRASCULA", 8, 1, outFile);

	writeByte(outFile, DRASCULA_DAT_VER);

	// Write charmap
	writeUint16BE(outFile, ARRAYSIZE(charMap));
	for (i = 0; i < ARRAYSIZE(charMap); i++) {
		writeByte(outFile, charMap[i].inChar);
		writeUint16BE(outFile, charMap[i].mappedChar);
		writeByte(outFile, charMap[i].charType);
	}

	// Write item locations
	writeUint16BE(outFile, ARRAYSIZE(itemLocations));
	for (i = 0; i < ARRAYSIZE(itemLocations); i++) {
		writeUint16BE(outFile, itemLocations[i].x);
		writeUint16BE(outFile, itemLocations[i].y);
	}

	// Write x_pol and y_pol
	writeUint16BE(outFile, ARRAYSIZE(x_pol));
	for (i = 0; i < ARRAYSIZE(x_pol); i++) {
		writeUint16BE(outFile, x_pol[i]);
		writeUint16BE(outFile, y_pol[i]);
	}

	// Write verbBarX
	writeUint16BE(outFile, ARRAYSIZE(verbBarX));
	for (i = 0; i < ARRAYSIZE(verbBarX); i++) {
		writeUint16BE(outFile, verbBarX[i]);
	}

	// Write x1d_menu and y1d_menu
	writeUint16BE(outFile, ARRAYSIZE(x1d_menu));
	for (i = 0; i < ARRAYSIZE(x1d_menu); i++) {
		writeUint16BE(outFile, x1d_menu[i]);
		writeUint16BE(outFile, y1d_menu[i]);
	}

	// Write frameX
	writeUint16BE(outFile, ARRAYSIZE(frameX));
	for (i = 0; i < ARRAYSIZE(frameX); i++) {
		writeUint16BE(outFile, frameX[i]);
	}

	// Write candleX and candleY
	writeUint16BE(outFile, ARRAYSIZE(candleX));
	for (i = 0; i < ARRAYSIZE(candleX); i++) {
		writeUint16BE(outFile, candleX[i]);
		writeUint16BE(outFile, candleY[i]);
	}

	// Write pianistX
	writeUint16BE(outFile, ARRAYSIZE(pianistX));
	for (i = 0; i < ARRAYSIZE(pianistX); i++) {
		writeUint16BE(outFile, pianistX[i]);
	}

	// Write drunkX
	writeUint16BE(outFile, ARRAYSIZE(drunkX));
	for (i = 0; i < ARRAYSIZE(drunkX); i++) {
		writeUint16BE(outFile, drunkX[i]);
	}

	// Write preupdates
	writeUint16BE(outFile, ARRAYSIZE(roomPreUpdates));

	for (i = 0; i < ARRAYSIZE(roomPreUpdates); i++) {
		writeUint16BE(outFile, roomPreUpdates[i].roomNum);
		writeUint16BE(outFile, roomPreUpdates[i].flag);
		writeUint16BE(outFile, roomPreUpdates[i].flagValue);
		writeUint16BE(outFile, roomPreUpdates[i].sourceX);
		writeUint16BE(outFile, roomPreUpdates[i].sourceY);
		writeUint16BE(outFile, roomPreUpdates[i].destX);
		writeUint16BE(outFile, roomPreUpdates[i].destY);
		writeUint16BE(outFile, roomPreUpdates[i].width);
		writeUint16BE(outFile, roomPreUpdates[i].height);
		writeUint16BE(outFile, roomPreUpdates[i].type);
	}

	// Write updates
	writeUint16BE(outFile, ARRAYSIZE(roomUpdates));

	for (i = 0; i < ARRAYSIZE(roomUpdates); i++) {
		writeUint16BE(outFile, roomUpdates[i].roomNum);
		writeUint16BE(outFile, roomUpdates[i].flag);
		writeUint16BE(outFile, roomUpdates[i].flagValue);
		writeUint16BE(outFile, roomUpdates[i].sourceX);
		writeUint16BE(outFile, roomUpdates[i].sourceY);
		writeUint16BE(outFile, roomUpdates[i].destX);
		writeUint16BE(outFile, roomUpdates[i].destY);
		writeUint16BE(outFile, roomUpdates[i].width);
		writeUint16BE(outFile, roomUpdates[i].height);
		writeUint16BE(outFile, roomUpdates[i].type);
	}

	// Write room actions
	writeUint16BE(outFile, ARRAYSIZE(roomActions));

	for (i = 0; i < ARRAYSIZE(roomActions); i++) {
		writeUint16BE(outFile, roomActions[i].room);
		writeUint16BE(outFile, roomActions[i].chapter);
		writeUint16BE(outFile, roomActions[i].action);
		writeUint16BE(outFile, roomActions[i].objectID);
		writeUint16BE(outFile, roomActions[i].speechID);
	}

	for (i = 0; i < DATAALIGNMENT; i++)
		padBuf[i] = 0;

	// langs
	writeUint16BE(outFile, NUM_LANGS);

	// Write _text
	writeUint16BE(outFile, NUM_TEXT);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXT; i++) {
			len += strlen(_text[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXT; i++) {
			writeUint16BE(outFile, strlen(_text[lang][i]));
			fwrite(_text[lang][i], strlen(_text[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textd
	writeUint16BE(outFile, NUM_TEXTD);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTD; i++) {
			len += strlen(_textd[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTD; i++) {
			writeUint16BE(outFile, strlen(_textd[lang][i]));
			fwrite(_textd[lang][i], strlen(_textd[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textb
	writeUint16BE(outFile, NUM_TEXTB);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTB; i++) {
			len += strlen(_textb[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTB; i++) {
			writeUint16BE(outFile, strlen(_textb[lang][i]));
			fwrite(_textb[lang][i], strlen(_textb[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textbj
	writeUint16BE(outFile, NUM_TEXTBJ);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTBJ; i++) {
			len += strlen(_textbj[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTBJ; i++) {
			writeUint16BE(outFile, strlen(_textbj[lang][i]));
			fwrite(_textbj[lang][i], strlen(_textbj[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _texte
	writeUint16BE(outFile, NUM_TEXTE);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTE; i++) {
			len += strlen(_texte[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTE; i++) {
			writeUint16BE(outFile, strlen(_texte[lang][i]));
			fwrite(_texte[lang][i], strlen(_texte[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _texti
	writeUint16BE(outFile, NUM_TEXTI);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTI; i++) {
			len += strlen(_texti[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTI; i++) {
			writeUint16BE(outFile, strlen(_texti[lang][i]));
			fwrite(_texti[lang][i], strlen(_texti[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textl
	writeUint16BE(outFile, NUM_TEXTL);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTL; i++) {
			len += strlen(_textl[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTL; i++) {
			writeUint16BE(outFile, strlen(_textl[lang][i]));
			fwrite(_textl[lang][i], strlen(_textl[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textp
	writeUint16BE(outFile, NUM_TEXTP);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTP; i++) {
			len += strlen(_textp[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTP; i++) {
			writeUint16BE(outFile, strlen(_textp[lang][i]));
			fwrite(_textp[lang][i], strlen(_textp[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}
  
	// Write _textt
	writeUint16BE(outFile, NUM_TEXTT);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTT; i++) {
			len += strlen(_textt[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTT; i++) {
			writeUint16BE(outFile, strlen(_textt[lang][i]));
			fwrite(_textt[lang][i], strlen(_textt[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textvb
	writeUint16BE(outFile, NUM_TEXTVB);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTVB; i++) {
			len += strlen(_textvb[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTVB; i++) {
			writeUint16BE(outFile, strlen(_textvb[lang][i]));
			fwrite(_textvb[lang][i], strlen(_textvb[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textsys
	writeUint16BE(outFile, NUM_TEXTSYS);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTSYS; i++) {
			len += strlen(_textsys[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTSYS; i++) {
			writeUint16BE(outFile, strlen(_textsys[lang][i]));
			fwrite(_textsys[lang][i], strlen(_textsys[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _texthis
	writeUint16BE(outFile, NUM_TEXTHIS);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTHIS; i++) {
			len += strlen(_texthis[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTHIS; i++) {
			writeUint16BE(outFile, strlen(_texthis[lang][i]));
			fwrite(_texthis[lang][i], strlen(_texthis[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textverbs
	writeUint16BE(outFile, NUM_TEXTVERBS);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTVERBS; i++) {
			len += strlen(_textverbs[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTVERBS; i++) {
			writeUint16BE(outFile, strlen(_textverbs[lang][i]));
			fwrite(_textverbs[lang][i], strlen(_textverbs[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textmisc
	writeUint16BE(outFile, NUM_TEXTMISC);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTMISC; i++) {
			len += strlen(_textmisc[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTMISC; i++) {
			writeUint16BE(outFile, strlen(_textmisc[lang][i]));
			fwrite(_textmisc[lang][i], strlen(_textmisc[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	// Write _textd1
	writeUint16BE(outFile, NUM_TEXTD1);
	for (lang = 0; lang < NUM_LANGS; lang++) {
		len = 0;
		for (i = 0; i < NUM_TEXTD1; i++) {
			len += strlen(_textd1[lang][i]) + 2;
			pad = len % DATAALIGNMENT;
			len += pad;
		}
		writeUint16BE(outFile, len);

		for (i = 0; i < NUM_TEXTD1; i++) {
			writeUint16BE(outFile, strlen(_textd1[lang][i]));
			fwrite(_textd1[lang][i], strlen(_textd1[lang][i]), 1, outFile);

			fwrite(padBuf, pad, 1, outFile);
		}
	}

	fclose(outFile);

	return 0;
}
