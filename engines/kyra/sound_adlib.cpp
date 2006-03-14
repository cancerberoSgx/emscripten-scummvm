/* ScummVM - Scumm Interpreter
 * Copyright (C) 2006 The ScummVM project
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
 */

#include "common/stdafx.h"
#include "common/system.h"
#include "common/mutex.h"
#include "common/timer.h"
#include "kyra/resource.h"
#include "kyra/sound.h"

#include "sound/mixer.h"
#include "sound/fmopl.h"
#include "sound/audiostream.h"

// Basic Adlib Programming:
// http://www.gamedev.net/reference/articles/article446.asp

#define CALLBACKS_PER_SECOND 72

namespace Kyra {

class AdlibDriver : public AudioStream {
public:
	AdlibDriver(Audio::Mixer *mixer);
	~AdlibDriver();

	int callback(int opcode, ...);
	void callback();

	// AudioStream API
	int readBuffer(int16 *buffer, const int numSamples) {
		int32 samplesLeft = numSamples;
		memset(buffer, 0, sizeof(int16) * numSamples);
		while (samplesLeft) {
			if (!_samplesTillCallback) {
				callback();
				_samplesTillCallback = _samplesPerCallback;
				_samplesTillCallbackRemainder += _samplesPerCallbackRemainder;
				if (_samplesTillCallbackRemainder >= CALLBACKS_PER_SECOND) {
					_samplesTillCallback++;
					_samplesTillCallbackRemainder -= CALLBACKS_PER_SECOND;
				}
			}

			int32 render = MIN(samplesLeft, _samplesTillCallback);
			samplesLeft -= render;
			_samplesTillCallback -= render;
			YM3812UpdateOne(_adlib, buffer, render);
			buffer += render;
		}
		return numSamples;
	}

	bool isStereo() const { return false; }
	bool endOfData() const { return false; }
	int getRate() const { return _mixer->getOutputRate(); }

private:
	struct OpcodeEntry {
		typedef int (AdlibDriver::*DriverOpcode)(va_list &list);
		DriverOpcode function;
		const char *name;
	};

	static const OpcodeEntry _opcodeList[];
	static const int _opcodesEntries;

	int snd_ret0x100(va_list &list);
	int snd_ret0x1983(va_list &list);
	int snd_initDriver(va_list &list);
	int snd_deinitDriver(va_list &list);
	int snd_setSoundData(va_list &list);
	int snd_unkOpcode1(va_list &list);
	int snd_startSong(va_list &list);
	int snd_unkOpcode2(va_list &list);
	int snd_unkOpcode3(va_list &list);
	int snd_readByte(va_list &list);
	int snd_writeByte(va_list &list);
	int snd_setUnk5(va_list &list);
	int snd_unkOpcode4(va_list &list);
	int snd_dummy(va_list &list);
	int snd_getNullvar4(va_list &list);
	int snd_setNullvar3(va_list &list);
	int snd_setFlag(va_list &list);
	int snd_clearFlag(va_list &list);

	// These variables have not yet been named, but some of them are partly
	// known nevertheless:
	//
	// unk4  - Unknown. Related to sound timing?
	// unk11 - Unknown. Used for updating random durations.
	// unk16 - Sound-related. Possibly some sort of pitch bend.
	// unk18 - Sound-effect. Used for secondaryEffect1()
	// unk19 - Sound-effect. Used for secondaryEffect1()
	// unk20 - Sound-effect. Used for secondaryEffect1()
	// unk21 - Sound-effect. Used for secondaryEffect1()
	// unk22 - Sound-effect. Used for secondaryEffect1()
	// unk29 - Sound-effect. Used for primaryEffect1()
	// unk30 - Sound-effect. Used for primaryEffect1()
	// unk31 - Sound-effect. Used for primaryEffect1()
	// unk32 - Sound-effect. Used for primaryEffect2()
	// unk33 - Sound-effect. Used for primaryEffect2()
	// unk34 - Sound-effect. Used for primaryEffect2()
	// unk35 - Sound-effect. Used for primaryEffect2()
	// unk36 - Sound-effect. Used for primaryEffect2()
	// unk37 - Sound-effect. Used for primaryEffect2()
	// unk38 - Sound-effect. Used for primaryEffect2()
	// unk39 - Currently unused, except for updateCallback56()
	// unk40 - Currently unused, except for updateCallback56()
	// unk41 - Sound-effect. Used for primaryEffect2()

	struct Channel {
		uint8 opExtraLevel2;
		uint8 *dataptr;
		uint8 duration;
		uint8 repeatCounter;
		int8 baseOctave;
		uint8 priority;
		uint8 dataptrStackPos;
		uint8 *dataptrStack[4];
		int8 baseNote;
		uint8 unk29;
		int8 unk31;
		uint16 unk30;
		uint16 unk37;
		uint8 unk33;
		uint8 unk34;
		uint8 unk35;
		uint8 unk36;
		int8 unk32;
		int8 unk41;
		uint8 unk38;
		uint8 opExtraLevel1;
		uint8 spacing2;
		uint8 baseFreq;
		int8 tempo;
		int8 position;
		uint8 regAx;
		uint8 regBx;
		typedef void (AdlibDriver::*Callback)(Channel&);
		Callback primaryEffect;
		Callback secondaryEffect;
		uint8 fractionalSpacing;
		uint8 opLevel1;
		uint8 opLevel2;
		uint8 opExtraLevel3;
		uint8 twoChan;
		uint8 unk39;	
		uint8 unk40;
		uint8 spacing1;
		uint8 unk11;
		uint8 unk19;
		int8 unk18;
		int8 unk20;
		int8 unk21;
		uint8 unk22;
		uint16 offset;
		uint8 tempoReset;
		uint8 rawNote;
		int8 unk16;
	};

	void primaryEffect1(Channel &channel);
	void primaryEffect2(Channel &channel);
	void secondaryEffect1(Channel &channel);

	void resetAdlibState();
	void writeOPL(byte reg, byte val);
	void initChannel(Channel &channel);
	void noteOff(Channel &channel);
	void unkOutput2(uint8 num);

	uint16 getRandomNr();
	void setupDuration(uint8 duration, Channel &channel);

	void setupNote(uint8 rawNote, Channel &channel, bool flag = false);
	void setupInstrument(uint8 regOffset, uint8 *dataptr, Channel &channel);
	void noteOn(Channel &channel);

	void adjustVolume(Channel &channel);

	uint8 calculateOpLevel1(Channel &channel);
	uint8 calculateOpLevel2(Channel &channel);

	uint16 checkValue(int16 val) {
		if (val < 0)
			val = 0;
		if (val > 0x3F)
			val = 0x3F;
		return val;
	}

	void callbackOutput();
	void callbackProcess();

	struct ParserOpcode {
		typedef int (AdlibDriver::*POpcode)(uint8 *&dataptr, Channel &channel, uint8 value);
		POpcode function;
		const char *name;
	};
	static const ParserOpcode _parserOpcodeTable[];
	static const int _parserOpcodeTableSize;

	int update_setRepeat(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_checkRepeat(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback3(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setNoteSpacing(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_jump(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_jumpToSubroutine(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_returnFromSubroutine(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setBaseOctave(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_stopChannel(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_playRest(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_writeAdlib(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback12(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setBaseNote(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setupSecondaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_stopOtherChannel(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback16(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setupInstrument(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setupPrimaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_removePrimaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setBaseFreq(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setupPrimaryEffect2(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setPriority(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback23(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback24(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setExtraLevel1(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback26(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_playNote(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setFractionalNoteSpacing(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setTempo(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_removeSecondaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setChannelTempo(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setExtraLevel3(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setExtraLevel2(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_changeExtraLevel2(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setAMDepth(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setVibratoDepth(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_changeExtraLevel1(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback38(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback39(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_removePrimaryEffect2(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback41(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_resetToGlobalTempo(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_nop1(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback44(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback45(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback46(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_nop2(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback48(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback49(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback50(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback51(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback52(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback53(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback54(uint8 *&dataptr, Channel &channel, uint8 value);
	int update_setTempoReset(uint8 *&dataptr, Channel &channel, uint8 value);
	int updateCallback56(uint8 *&dataptr, Channel &channel, uint8 value);
private:
	// These variables have not yet been named, but some of them are partly
	// known nevertheless:
	//
	// _unk4           - Unknown, but probably indicates that Adlib's
	//                   rhythm section is active.
	// _unk5           - Currently unused, except for updateCallback54()
	// _unkValue1      - Unknown. Used for updating _unkValue2
	// _unkValue2      - Unknown. Used for updating _unkValue4
	// _unkValue3      - Unknown. Used for updating _unkValue2
	// _unkValue4      - Unknown. Used for updating _unkValue5
	// _unkValue5      - Unknown. Used for controlling updateCallback24().
	// _unkValue6      - Unknown. Something to do with channel 1 volume?
	// _unkValue7      - Unknown. Something to do with channel 2 volume?
	// _unkValue8      - Unknown. Something to do with channel 2 volume?
	// _unkValue9      - Unknown. Something to do with channel 3 volume?
	// _unkValue10     - Unknown. Something to do with channel 3 volume?
	// _unkValue11     - Unknown. Something to do with channel 2 volume?
	// _unkValue12     - Unknown. Something to do with channel 2 volume?
	// _unkValue13     - Unknown. Something to do with channel 3 volume?
	// _unkValue14     - Unknown. Something to do with channel 3 volume?
	// _unkValue15     - Unknown. Something to do with channel 3 volume?
	// _unkValue16     - Unknown. Something to do with channel 3 volume?
	// _unkValue17     - Unknown. Something to do with channel 2 volume?
	// _unkValue18     - Unknown. Something to do with channel 2 volume?
	// _unkValue19     - Unknown. Something to do with channel 1 volume?
	// _unkValue20     - Unknown. Something to do with channel 1 volume?
	// _unkOutputByte2 - Unknown. Something to do with the BD register.
	// _unkTable[]     - Probably frequences for the 12-tone scale.
	// _unkTable2[]    - Unknown. Currently only used by updateCallback46()
	// _unkTable2_1[]  - One of the tables in _unkTable2[]
	// _unkTable2_2[]  - One of the tables in _unkTable2[]
	// _unkTable2_3[]  - One of the tables in _unkTable2[]

	int32 _samplesPerCallback;
	int32 _samplesPerCallbackRemainder;
	int32 _samplesTillCallback;
	int32 _samplesTillCallbackRemainder;

	int _lastProcessed;
	int8 _flagTrigger;
	int _curChannel;
	uint8 _unk4;
	uint8 _unk5;
	int _soundsPlaying;

	uint16 _rnd;

	uint8 _unkValue1;
	uint8 _unkValue2;
	int8 _unkValue3;
	uint8 _unkValue4;
	uint8 _unkValue5;
	uint8 _unkValue6;
	uint8 _unkValue7;
	uint8 _unkValue8;
	uint8 _unkValue9;
	uint8 _unkValue10;
	uint8 _unkValue11;
	uint8 _unkValue12;
	uint8 _unkValue13;
	uint8 _unkValue14;
	uint8 _unkValue15;
	uint8 _unkValue16;
	uint8 _unkValue17;
	uint8 _unkValue18;
	uint8 _unkValue19;
	uint8 _unkValue20;

	int _flags;
	FM_OPL *_adlib;

	uint8 *_soundData;

	uint8 _soundIdTable[0x10];
	Channel _channels[10];

	uint8 _unkOutputByte2;
	uint8 _curRegOffset;
	int8 _tempo;

	const uint8 *_tablePtr1;
	const uint8 *_tablePtr2;

	static const uint8 _regOffset[];
	static const uint16 _unkTable[];
	static const uint8 *_unkTable2[];
	static const uint8 _unkTable2_1[];
	static const uint8 _unkTable2_2[];
	static const uint8 _unkTable2_3[];
	static const uint8 _unkTables[][32];

	Common::Mutex _mutex;
	Audio::Mixer *_mixer;

	void lock() { _mutex.lock(); }
	void unlock() { _mutex.unlock(); }
};

AdlibDriver::AdlibDriver(Audio::Mixer *mixer) {
	_mixer = mixer;

	_flags = 0;
	_adlib = makeAdlibOPL(getRate());
	assert(_adlib);

	memset(_channels, 0, sizeof(_channels));
	_soundData = 0;

	_unkOutputByte2 = _curRegOffset = 0;

	_lastProcessed = _flagTrigger = _curChannel = _unk4 = 0;
	_rnd = 0x1234;

	_tempo = 0;

	_unkValue3 = -1;
	_unkValue1 = _unkValue2 = _unkValue4 = _unkValue5 = 0;
	_unkValue6 = _unkValue7 = _unkValue8 = _unkValue9 = _unkValue10 = 0;
	_unkValue11 = _unkValue12 = _unkValue13 = _unkValue14 = _unkValue15 =
	_unkValue16 = _unkValue17 = _unkValue18 = _unkValue19 = _unkValue20 = 0;

	_tablePtr1 = _tablePtr2 = 0;

	_mixer->setupPremix(this);

	_samplesPerCallback = getRate() / CALLBACKS_PER_SECOND;
	_samplesPerCallbackRemainder = getRate() % CALLBACKS_PER_SECOND;
	_samplesTillCallback = 0;
	_samplesTillCallbackRemainder = 0;
}

AdlibDriver::~AdlibDriver() {
	_mixer->setupPremix(0);
	OPLDestroy(_adlib);
	_adlib = 0;
}

int AdlibDriver::callback(int opcode, ...) {
	lock();
	if (opcode >= _opcodesEntries || opcode < 0) {
		warning("AdlibDriver: calling unknown opcode '%d'", opcode);
		return 0;
	}

	debugC(9, kDebugLevelSound, "Calling opcode '%s' (%d)", _opcodeList[opcode].name, opcode);

	va_list args;
	va_start(args, opcode);
	int returnValue = (this->*(_opcodeList[opcode].function))(args);
	va_end(args);
	unlock();
	return returnValue;
}

// Opcodes

int AdlibDriver::snd_ret0x100(va_list &list) {
	return 0x100;
}

int AdlibDriver::snd_ret0x1983(va_list &list) {
	return 0x1983;
}

int AdlibDriver::snd_initDriver(va_list &list) {
	_lastProcessed = _soundsPlaying = 0;
	resetAdlibState();
	return 0;
}

int AdlibDriver::snd_deinitDriver(va_list &list) {
	resetAdlibState();
	return 0;
}

int AdlibDriver::snd_setSoundData(va_list &list) {
	if (_soundData) {
		delete [] _soundData;
		_soundData = 0;
	}
	_soundData = va_arg(list, uint8*);
	return 0;
}

int AdlibDriver::snd_unkOpcode1(va_list &list) {
	warning("unimplemented snd_unkOpcode1");
	return 0;
}

int AdlibDriver::snd_startSong(va_list &list) {
	int songId = va_arg(list, int);
	_flags |= 8;
	_flagTrigger = 1;
	uint16 offset = READ_LE_UINT16(&_soundData[songId << 1]);
	uint8 firstByte = *(_soundData + offset);

	if ((songId << 1) != 0) {
		if (firstByte == 9) {
			if (_flags & 2)
				return 0;
		} else {
			if (_flags & 1)
				return 0;
		}
	}

	_soundIdTable[_soundsPlaying] = songId;
	++_soundsPlaying;
	_soundsPlaying &= 0x0F;

	return 0;
}

int AdlibDriver::snd_unkOpcode2(va_list &list) {
	warning("unimplemented snd_unkOpcode2");
	return 0;
}

int AdlibDriver::snd_unkOpcode3(va_list &list) {
	int value = va_arg(list, int);
	int loop = value;
	if (value < 0) {
		value = 0;
		loop = 9;
	}
	loop -= value;
	++loop;

	while (loop--) {
		_curChannel = value;
		Channel &channel = _channels[_curChannel];
		channel.priority = 0;
		channel.dataptr = 0;
		if (value != 9) {
			noteOff(channel);
		}
		++value;
	}

	return 0;
}

int AdlibDriver::snd_readByte(va_list &list) {
	int a = va_arg(list, int);
	int b = va_arg(list, int);
	uint8 *ptr = _soundData + READ_LE_UINT16(&_soundData[a << 1]) + b;
	return *ptr;
}

int AdlibDriver::snd_writeByte(va_list &list) {
	int a = va_arg(list, int);
	int b = va_arg(list, int);
	int c = va_arg(list, int);
	uint8 *ptr = _soundData + READ_LE_UINT16(&_soundData[a << 1]) + b;
	uint8 oldValue = *ptr;
	*ptr = (uint8)c;
	return oldValue;
}

int AdlibDriver::snd_setUnk5(va_list &list) {
	warning("unimplemented snd_setUnk5");
	return 0;
}

int AdlibDriver::snd_unkOpcode4(va_list &list) {
	warning("unimplemented snd_unkOpcode4");
	return 0;
}

int AdlibDriver::snd_dummy(va_list &list) {
	return 0;
}

int AdlibDriver::snd_getNullvar4(va_list &list) {
	warning("unimplemented snd_getNullvar4");
	return 0;
}

int AdlibDriver::snd_setNullvar3(va_list &list) {
	warning("unimplemented snd_setNullvar3");
	return 0;
}

int AdlibDriver::snd_setFlag(va_list &list) {
	int oldFlags = _flags;
	_flags |= va_arg(list, int);
	return oldFlags;
}

int AdlibDriver::snd_clearFlag(va_list &list) {
	int oldFlags = _flags;
	_flags &= ~(va_arg(list, int));
	return oldFlags;
}

// timer callback

void AdlibDriver::callback() {
	lock();
	--_flagTrigger;
	if (_flagTrigger < 0)
		_flags &= ~8;
	callbackOutput();
	callbackProcess();

	int8 temp = _unkValue3;
	_unkValue3 += _tempo;
	if (_unkValue3 < temp) {
		if (!(--_unkValue2)) {
			_unkValue2 = _unkValue1;
			++_unkValue4;
		}
	}
	unlock();
}

void AdlibDriver::callbackOutput() {
	while (_lastProcessed != _soundsPlaying) {
		uint8 *ptr = _soundData;

		ptr += READ_LE_UINT16(&ptr[_soundIdTable[_lastProcessed] << 1]);
		uint8 chan = *ptr++;
		Channel &channel = _channels[chan];

		uint8 priority = *ptr++;

		// Only start this sound if its priority is higher than the one
		// already playing.

		if (priority >= channel.priority) {
			initChannel(channel);
			channel.priority = priority;
			channel.dataptr = ptr;
			channel.tempo = -1;
			channel.position = -1;
			channel.duration = 1;
			if (chan != 9) {
				unkOutput2(chan);
			}
		}

		++_lastProcessed;
		_lastProcessed &= 0x0F;
	}
}

// A few words on opcode parsing and timing:
//
// First of all, We simulate a timer callback 72 times per second. Each timeout
// we update each channel that has something to play.
//
// Each channel has its own individual tempo, which is added to its position.
// This will frequently cause the position to "wrap around" but that is
// intentional. In fact, it's the signal to go ahead and do more stuff with
// that channel.
//
// Each channel also has a duration, indicating how much time is left on the
// its current task. This duration is decreased by one. As long as it still has
// not reached zero, the only thing that can happen is that the note is turned
// off depending on manual or automatic note spacing. Once the duration reaches
// zero, a new set of musical opcodes are executed.
//
// An opcode is one byte, followed by a variable number of parameters. Since
// most opcodes have at least one one-byte parameter, we read that as well. Any
// opcode that doesn't have that one parameter is responsible for moving the
// data pointer back again.
//
// If the most significant bit of the opcode is 1, it's a function; call it.
// The opcode functions return either 0 (continue), 1 (stop) or 2 (stop, and do
// not run the effects callbacks).
//
// If the most significant bit of the opcode is 0, it's a note, and the first
// parameter is its duration. (There are cases where the duration is modified
// but that's an exception.) The note opcode is assumed to return 1, and is the
// last opcode unless its duration is zero.
//
// Finally, most of the times that the callback is called, it will invoke the
// effects callbacks. The final opcode in a set can prevent this, if it's a
// function and it returns anything other than 1.

void AdlibDriver::callbackProcess() {
	for (_curChannel = 9; _curChannel >= 0; --_curChannel) {
		int result = 1;

		if (!_channels[_curChannel].dataptr) {
			continue;
		}
	
		Channel &channel = _channels[_curChannel];
		_curRegOffset = _regOffset[_curChannel];

		if (channel.tempoReset) {
			channel.tempo = _tempo;
		}

		int8 backup = channel.position;
		channel.position += channel.tempo;
		if (channel.position < backup) {
			if (--channel.duration) {
				if (channel.duration == channel.spacing2)
					noteOff(channel);
				if (channel.duration == channel.spacing1 && _curChannel != 9)
					noteOff(channel);
			} else {
				while (channel.dataptr) {
					uint8 opcode = *channel.dataptr++;
					uint8 param = *channel.dataptr++;

					if (opcode & 0x80) {
						opcode &= 0x7F;
						if (opcode >= _parserOpcodeTableSize)
							opcode = _parserOpcodeTableSize - 1;
						debugC(9, kDebugLevelSound, "Calling opcode '%s' (%d) (channel: %d)", _parserOpcodeTable[opcode].name, opcode, _curChannel);
						result = (this->*(_parserOpcodeTable[opcode].function))(channel.dataptr, channel, param);
						if (result)
							break;
					} else {
						debugC(9, kDebugLevelSound, "Note on opcode 0x%02X (duration: %d) (channel: %d)", opcode, param, _curChannel);
						setupNote(opcode, channel);
						noteOn(channel);
						setupDuration(param, channel);
						if (param)
							break;
					}
				}
			}
		}

		if (result == 1) {
			if (channel.primaryEffect)
				(this->*(channel.primaryEffect))(channel);
			if (channel.secondaryEffect)
				(this->*(channel.secondaryEffect))(channel);
		}
	}
}

// 

void AdlibDriver::resetAdlibState() {
	debugC(9, kDebugLevelSound, "resetAdlibState()");
	_rnd = 0x1234;

	// Authorize the control of the waveforms
	writeOPL(0x01, 0x20);

	// Select FM music mode
	writeOPL(0x08, 0x00);

	// I would guess the main purpose of this is to turn off the rhythm,
	// thus allowing us to use 9 melodic voices instead of 6.
	writeOPL(0xBD, 0x00);

	int loop = 10;
	while (loop--) {
		if (loop != 9) {
			// Silence the channel
			writeOPL(0x40 + _regOffset[loop], 0x3F);
			writeOPL(0x43 + _regOffset[loop], 0x3F);
		}
		initChannel(_channels[loop]);
	}
}

// Old calling style: output0x388(0xABCD)
// New calling style: writeOPL(0xAB, 0xCD)

void AdlibDriver::writeOPL(byte reg, byte val) {
	OPLWriteReg(_adlib, reg, val);
}

void AdlibDriver::initChannel(Channel &channel) {
	debugC(9, kDebugLevelSound, "initChannel(%d)", &channel - _channels);
	memset(&channel.dataptr, 0, sizeof(Channel) - ((char*)&channel.dataptr - (char*)&channel));

	channel.tempo = -1;
	channel.priority = 0;
	// normally here are nullfuncs but we set 0 for now
	channel.primaryEffect = 0;
	channel.secondaryEffect = 0;
	channel.spacing1 = 1;
}

void AdlibDriver::noteOff(Channel &channel) {
	debugC(9, kDebugLevelSound, "noteOff(%d)", &channel - _channels);

	// I believe that 9 is the percussion channel.
	if (_curChannel == 9)
		return;

	// I believe this has to do with channels 6, 7, and 8 being special
	// when Adlib's rhythm section is enabled.
	if (_unk4 && _curChannel >= 6)
		return;

	// This means the "Key On" bit will always be 0
	channel.regBx &= 0xDF;

	// Octave / F-Number / Key-On
	writeOPL(0xB0 + _curChannel, channel.regBx);
}

void AdlibDriver::unkOutput2(uint8 chan) {
	debugC(9, kDebugLevelSound, "unkOutput2(%d)", chan);

	// I believe this has to do with channels 6, 7, and 8 being special
	// when Adlib's rhythm section is enabled.

	if (_unk4 && chan >= 6)
		return;

	uint8 offset = _regOffset[chan];

	// The channel is cleared: First the attack/delay rate, then the
	// sustain/release rate, and finally the note is turned off.

	writeOPL(0x60 + offset, 0xFF);
	writeOPL(0x63 + offset, 0xFF);

	writeOPL(0x80 + offset, 0xFF);
	writeOPL(0x83 + offset, 0xFF);

	writeOPL(0xB0 + chan, 0x00);

	// FIXME!
	//
	// ...and then the note is turned on again, with whatever value is
	// still lurking in the A0 + chan register, but everything else -
	// including the two most significant frequency bit, and the octave -
	// set to zero.
	//
	// This is very strange behaviour, and appears to be the cause of the
	// bug where low-frequent notes are played at the beginning of a new
	// sound. However, this is what the original does, and the bug does not
	// seem to happen with current versions of the FMOPL code.
	//
	// Unfortunately, we cannot use more recent versions because of license
	// incompatibilities.
	//
	// Ken Silverman's Adlib emulator (which can be found on his Web page -
	// http://www.advsys.net/ken - and as part of AdPlug) also seems to be
	// proof against this particular bug, but is apparently not as feature
	// complete as MAME's.

	writeOPL(0xB0 + chan, 0x20);
}

// I believe this is a random number generator. It actually does seem to
// generate an even distribution of almost all numbers from 0 through 65535,
// though in my tests some numbers were never generated.

uint16 AdlibDriver::getRandomNr() {
	_rnd += 0x9248;
	uint16 lowBits = _rnd & 7;
	_rnd >>= 3;
	_rnd |= (lowBits << 13);
	return _rnd;
}

void AdlibDriver::setupDuration(uint8 duration, Channel &channel) {
	debugC(9, kDebugLevelSound, "setupDuration(%d, %d)", duration, &channel - _channels);
	if (channel.unk11) {
		channel.duration = duration + (getRandomNr() & channel.unk11);
		return;
	}
	if (channel.fractionalSpacing) {
		channel.spacing2 = (duration >> 3) * channel.fractionalSpacing;
	}
	channel.duration = duration;
}

// This function may or may not play the note. It's usually followed by a call
// to noteOn(), which will always play the current note.

void AdlibDriver::setupNote(uint8 rawNote, Channel &channel, bool flag) {
	debugC(9, kDebugLevelSound, "setupNote(%d, %d)", rawNote, &channel - _channels);

	channel.rawNote = rawNote;

	int8 note = (rawNote & 0x0F) + channel.baseNote;
	int8 octave = ((rawNote + channel.baseOctave) >> 4) & 0x0F;

	// There are only twelve notes. If we go outside that, we have to
	// adjust the note and octave.

	if (note >= 12) {
		note -= 12;
		octave++;
	} else if (note < 0) {
		note += 12;
		octave--;
	}

	// The calculation of frequency looks quite different from the original
	// disassembly at a first glance, but when you consider that the
	// largest possible value would be 0x0246 + 0xFF + 0x47 (and that's if
	// baseFreq is unsigned), freq is still a 10-bit value, just as it
	// should be to fit in the Ax and Bx registers.
	//
	// If it were larger than that, it could have overflowed into the
	// octave bits, and that could possibly have been used in some sound.
	// But as it is now, I can't see any way it would happen.

	uint16 freq = _unkTable[note] + channel.baseFreq;

	// When called from callback 41, the behaviour is slightly different:
	// We adjust the frequency, even when channel.unk16 is 0.

	if (channel.unk16 || flag) {
		const uint8 *table;

		if (channel.unk16 >= 0) {
			table = _unkTables[(channel.rawNote & 0x0F) + 2];
			freq += table[channel.unk16];
		} else {
			table = _unkTables[channel.rawNote & 0x0F];
			freq -= table[-channel.unk16];
		}
	}

	channel.regAx = freq & 0xFF;
	channel.regBx = (channel.regBx & 0x20) | (octave << 2) | ((freq >> 8) & 0x03);

	// Keep the note on or off
	writeOPL(0xA0 + _curChannel, channel.regAx);
	writeOPL(0xB0 + _curChannel, channel.regBx);
}

void AdlibDriver::setupInstrument(uint8 regOffset, uint8 *dataptr, Channel &channel) {
	debugC(9, kDebugLevelSound, "setupInstrument(%d, %p, %d)", regOffset, (const void *)dataptr, &channel - _channels);
	// Amplitude Modulation / Vibrato / Envelope Generator Type /
	// Keyboard Scaling Rate / Modulator Frequency Multiple
	writeOPL(0x20 + regOffset, *dataptr++);
	writeOPL(0x23 + regOffset, *dataptr++);

	uint8 temp = *dataptr++;

	// Feedback / Algorithm

	// It is very likely that _curChannel really does refer to the same
	// channel as regOffset, but there's only one Cx register per channel.

	writeOPL(0xC0 + _curChannel, temp);

	// The algorithm bit. I don't pretend to understand this fully, but
	// "If set to 0, operator 1 modulates operator 2. In this case,
	// operator 2 is the only one producing sound. If set to 1, both
	// operators produce sound directly. Complex sounds are more easily
	// created if the algorithm is set to 0."

	channel.twoChan = temp & 1;

	// Waveform Select
	writeOPL(0xE0 + regOffset, *dataptr++);
	writeOPL(0xE3 + regOffset, *dataptr++);

	channel.opLevel1 = *dataptr++;
	channel.opLevel2 = *dataptr++;

	// Level Key Scaling / Total Level
	writeOPL(0x40 + regOffset, calculateOpLevel1(channel));
	writeOPL(0x43 + regOffset, calculateOpLevel2(channel));

	// Attack Rate / Decay Rate
	writeOPL(0x60 + regOffset, *dataptr++);
	writeOPL(0x63 + regOffset, *dataptr++);

	// Sustain Level / Release Rate
	writeOPL(0x80 + regOffset, *dataptr++);
	writeOPL(0x83 + regOffset, *dataptr++);
}

// Apart from playing the note, this function also updates the variables for
// primary effect 2.

void AdlibDriver::noteOn(Channel &channel) {
	debugC(9, kDebugLevelSound, "noteOn(%d)", &channel - _channels);

	// The "note on" bit is set, and the current note is played.

	channel.regBx |= 0x20;
	writeOPL(0xB0 + _curChannel, channel.regBx);

	int8 shift = 9 - channel.unk33;
	uint16 temp = channel.regAx | (channel.regBx << 8);
	channel.unk37 = ((temp & 0x3FF) >> shift) & 0xFF;
	channel.unk38 = channel.unk36;
}

void AdlibDriver::adjustVolume(Channel &channel) {
	debugC(9, kDebugLevelSound, "adjustVolume(%d)", &channel - _channels);
	// Level Key Scaling / Total Level

	writeOPL(0x43 + _regOffset[_curChannel], calculateOpLevel2(channel));
	if (channel.twoChan)
		writeOPL(0x40 + _regOffset[_curChannel], calculateOpLevel1(channel));
}

// This is presumably only used for some sound effects, e.g. Malcolm blowing up
// the trees in the intro (but not the effect where he "booby-traps" the big
// tree) and turning Kallak to stone. Related functions and variables:
//
// update_setupPrimaryEffect1()
//    - Initialises unk29, unk30 and unk31
//    - unk29 is not further modified
//    - unk30 is not further modified, except by update_removePrimaryEffect1()
//
// update_removePrimaryEffect1()
//    - Deinitialises unk30
//
// unk29 - determines how often the notes are played
// unk30 - modifies the frequency
// unk31 - determines how often the notes are played

void AdlibDriver::primaryEffect1(Channel &channel) {
	debugC(9, kDebugLevelSound, "Calling primaryEffect1 (channel: %d)", _curChannel);
	int8 temp = channel.unk31;
	channel.unk31 += channel.unk29;
	if (channel.unk31 >= temp)
		return;

	// Initialise unk1 to the current frequency
	uint16 unk1 = ((channel.regBx & 3) << 8) | channel.regAx;

	// This is presumably to shift the "note on" bit so far to the left
	// that it won't be affected by any of the calculations below.
	uint16 unk2 = ((channel.regBx & 0x20) << 8) | (channel.regBx & 0x1C);

	int16 unk3 = (int16)channel.unk30;

	if (unk3 >= 0) {
		unk1 += unk3;
		if (unk1 >= 734) {
			// The new frequency is too high. Shift it down and go
			// up one octave.
			unk1 >>= 1;
			if (!(unk1 & 0x3FF))
				++unk1;
			unk2 += 4;
			unk2 &= 0xFF1C;
		}
	} else {
		unk1 += unk3;
		if (unk1 < 388) {
			// The new frequency is too low. Shift it up and go
			// down one octave.
			unk1 <<= 1;
			if (!(unk1 & 0x3FF))
				--unk1;
			unk2 -= 4;
			unk2 &= 0xFF1C;
		}
	}

	// Make sure that the new frequency is still a 10-bit value.
	unk1 &= 0x3FF;

	writeOPL(0xA0 + _curChannel, unk1 & 0xFF);
	channel.regAx = unk1 & 0xFF;

	// Shift down the "note on" bit again.
	uint8 value = unk1 >> 8;
	value |= (unk2 >> 8) & 0xFF;
	value |= unk2 & 0xFF;

	writeOPL(0xB0 + _curChannel, value);
	channel.regBx = value;
}

// This is presumably only used for some sound effects, e.g. Malcolm entering
// and leaving Kallak's hut. Related functions and variables:
//
// update_setupPrimaryEffect2()
//    - Initialises unk32, unk33, unk34, unk35 and unk36
//    - unk32 is not further modified
//    - unk33 is not further modified
//    - unk34 is a countdown that gets reinitialised to unk35 on zero
//    - unk35 is based on unk34 and not further modified
//    - unk36 is not further modified
//
// noteOn()
//    - Plays the current note
//    - Updates unk37 with a new (lower?) frequency
//    - Copies unk36 to unk38. The unk38 variable is a countdown.
//
// unk32 - determines how often the notes are played
// unk33 - modifies the frequency
// unk34 - countdown, updates frequency on zero
// unk35 - initialiser for unk34 countdown
// unk36 - initialiser for unk38 countdown
// unk37 - frequency
// unk38 - countdown, begins playing on zero
// unk41 - determines how often the notes are played
//
// Note that unk41 is never initialised. Not that it should matter much, but it
// is a bit sloppy.

void AdlibDriver::primaryEffect2(Channel &channel) {
	debugC(9, kDebugLevelSound, "Calling primaryEffect2 (channel: %d)", _curChannel);
	if (channel.unk38) {
		--channel.unk38;
		return;
	}

	int8 temp = channel.unk41;
	channel.unk41 += channel.unk32;
	if (channel.unk41 < temp) {
		uint16 unk1 = channel.unk37;
		if (!(--channel.unk34)) {
			unk1 ^= 0xFFFF;
			++unk1;
			channel.unk37 = unk1;
			channel.unk34 = channel.unk35;
		}

		uint16 unk2 = (channel.regAx | (channel.regBx << 8)) & 0x3FF;
		unk2 += unk1;
		
		channel.regAx = unk2 & 0xFF;
		channel.regBx = (channel.regBx & 0xFC) | (unk2 >> 8);

		// Octave / F-Number / Key-On
		writeOPL(0xA0 + _curChannel, channel.regAx);
		writeOPL(0xB0 + _curChannel, channel.regBx);
	}
}

// I don't know where this is used. The same operation is performed several
// times on the current channel, using a chunk of the _soundData[] buffer for
// parameters. The parameters are used starting at the end of the chunk.
//
// Since we use _curRegOffset to specify the final register, it's quite
// unlikely that this function is ever used to play notes. It's probably only
// used to modify the sound. Another thing that supports this idea is that it
// can be combined with any of the effects callbacks above.
//
// Related functions and variables:
//
// update_setupSecondaryEffect1()
//    - Initialies unk18, unk19, unk20, unk21, unk22 and offset
//    - unk19 is not further modified
//    - unk20 is not further modified
//    - unk22 is not further modified
//    - offset is not further modified
//
// unk18 -  determines how often the operation is performed
// unk19 -  determines how often the operation is performed
// unk20 -  the start index into the data chunk
// unk21 -  the current index into the data chunk
// unk22 -  the operation to perform
// offset - the offset to the data chunk

void AdlibDriver::secondaryEffect1(Channel &channel) {
	debugC(9, kDebugLevelSound, "Calling secondaryEffect1 (channel: %d)", _curChannel);
	int8 temp = channel.unk18;
	channel.unk18 += channel.unk19;
	if (channel.unk18 < temp) {
		if (--channel.unk21 < 0) {
			channel.unk21 = channel.unk20;
		}
		writeOPL(channel.unk22 + _curRegOffset, _soundData[channel.offset + channel.unk21]);
	}
}

uint8 AdlibDriver::calculateOpLevel1(Channel &channel) {
	int8 value = channel.opLevel1 & 0x3F;

	if (channel.twoChan) {
		value += channel.opExtraLevel1;
		value += channel.opExtraLevel2;
		value += channel.opExtraLevel3;
	}

	// Don't allow the total level to overflow into the scaling level bits.

	if (value > 0x3F) {
		value = 0x3F;
	} else if (value < 0)
		value = 0;

	// Preserve the scaling level bits from opLevel1

	return value | (channel.opLevel1 & 0xC0);
}

uint8 AdlibDriver::calculateOpLevel2(Channel &channel) {
	int8 value = channel.opLevel2 & 0x3F;

	value += channel.opExtraLevel1;
	value += channel.opExtraLevel2;
	value += channel.opExtraLevel3;

	// Don't allow the total level to overflow into the scaling level bits.

	if (value > 0x3F) {
		value = 0x3F;
	} else if (value < 0)
		value = 0;

	// Preserve the scaling level bits from opLevel2

	return value | (channel.opLevel2 & 0xC0);
}

// parser opcodes

int AdlibDriver::update_setRepeat(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.repeatCounter = value;
	return 0;
}

int AdlibDriver::update_checkRepeat(uint8 *&dataptr, Channel &channel, uint8 value) {
	++dataptr;
	if (--channel.repeatCounter) {
		int16 add = READ_LE_UINT16(dataptr - 2);
		dataptr += add;
	}
	return 0;
}

// This is similar to callbackOutput()

int AdlibDriver::updateCallback3(uint8 *&dataptr, Channel &channel, uint8 value) {
	if (value == 0xFF)
		return 0;

	uint16 add = value << 1;
	uint8 *ptr = _soundData + READ_LE_UINT16(_soundData + add);
	uint8 chan = *ptr++;
	Channel &channel2 = _channels[chan];
	uint8 priority = *ptr++;
	if (priority >= channel2.priority) {
		_flagTrigger = 1;
		_flags |= 8;
		initChannel(channel2);
		channel2.priority = priority;
		channel2.dataptr = ptr;
		channel2.tempo = -1;
		channel2.position = -1;
		channel2.duration = 1;
		unkOutput2(chan);
	}
	return 0;
}

int AdlibDriver::update_setNoteSpacing(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.spacing1 = value;
	return 0;
}

int AdlibDriver::update_jump(uint8 *&dataptr, Channel &channel, uint8 value) {
	--dataptr;
	int16 add = READ_LE_UINT16(dataptr); dataptr += 2;
	dataptr += add;
	return 0;
}

int AdlibDriver::update_jumpToSubroutine(uint8 *&dataptr, Channel &channel, uint8 value) {
	--dataptr;
	int16 add = READ_LE_UINT16(dataptr); dataptr += 2;
	channel.dataptrStack[channel.dataptrStackPos++] = dataptr;
	dataptr += add;
	return 0;
}

int AdlibDriver::update_returnFromSubroutine(uint8 *&dataptr, Channel &channel, uint8 value) {
	dataptr = channel.dataptrStack[--channel.dataptrStackPos];
	return 0;
}

int AdlibDriver::update_setBaseOctave(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.baseOctave = value;
	return 0;
}

int AdlibDriver::update_stopChannel(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.priority = 0;
	if (_curChannel != 9) {
		noteOff(channel);
	}
	dataptr = 0;
	return 2;
}

int AdlibDriver::update_playRest(uint8 *&dataptr, Channel &channel, uint8 value) {
	setupDuration(value, channel);
	noteOff(channel);
	return (value != 0);
}

int AdlibDriver::update_writeAdlib(uint8 *&dataptr, Channel &channel, uint8 value) {
	writeOPL(value, *dataptr++);
	return 0;
}

int AdlibDriver::updateCallback12(uint8 *&dataptr, Channel &channel, uint8 value) {
	setupNote(value, channel);
	value = *dataptr++;
	setupDuration(value, channel);
	return (value != 0);
}

int AdlibDriver::update_setBaseNote(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.baseNote = value;
	return 0;
}

int AdlibDriver::update_setupSecondaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.unk18 = value;
	channel.unk19 = value;
	channel.unk20 = channel.unk21 = *dataptr++;
	channel.unk22 = *dataptr++;
	channel.offset = READ_LE_UINT16(dataptr); dataptr += 2;
	channel.secondaryEffect = &AdlibDriver::secondaryEffect1;
	return 0;
}

int AdlibDriver::update_stopOtherChannel(uint8 *&dataptr, Channel &channel, uint8 value) {
	Channel &channel2 = _channels[value];
	channel2.duration = 0;
	channel2.priority = 0;
	channel2.dataptr = 0;
	return 0;
}

int AdlibDriver::updateCallback16(uint8 *&dataptr, Channel &channel, uint8 value) {
	uint8 *ptr = _soundData;
	ptr += READ_LE_UINT16(&_soundData[value << 1]);
	Channel &channel2 = _channels[*ptr];
	if (!channel2.dataptr) {
		return 0;
	}
	dataptr -= 2;
	return 2;
}

int AdlibDriver::update_setupInstrument(uint8 *&dataptr, Channel &channel, uint8 value) {
	uint8 *ptr = _soundData;
	ptr += READ_LE_UINT16(_soundData + (value << 1) + 0x1F4);
	setupInstrument(_curRegOffset, ptr, channel);
	return 0;
}

int AdlibDriver::update_setupPrimaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.unk29 = value;
	channel.unk30 = READ_BE_UINT16(dataptr);
	dataptr += 2;
	channel.primaryEffect = &AdlibDriver::primaryEffect1;
	channel.unk31 = -1;
	return 0;
}

int AdlibDriver::update_removePrimaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value) {
	--dataptr;
	channel.primaryEffect = 0;
	channel.unk30 = 0;
	return 0;
}

int AdlibDriver::update_setBaseFreq(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.baseFreq = value;
	return 0;
}

int AdlibDriver::update_setupPrimaryEffect2(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.unk32 = (int8)value;
	channel.unk33 = *dataptr++;
	uint8 temp = *dataptr++;
	channel.unk34 = temp + 1;
	channel.unk35 = temp << 1;
	channel.unk36 = *dataptr++;
	channel.primaryEffect = &AdlibDriver::primaryEffect2;
	return 0;
}

int AdlibDriver::update_setPriority(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.priority = value;
	return 0;
}

int AdlibDriver::updateCallback23(uint8 *&dataptr, Channel &channel, uint8 value) {
	value >>= 1;
	_unkValue1 = _unkValue2 = value;
	_unkValue3 = -1;
	_unkValue4 = _unkValue5 = 0;
	return 0;
}

int AdlibDriver::updateCallback24(uint8 *&dataptr, Channel &channel, uint8 value) {
	if (_unkValue5) {
		if (_unkValue4 & value) {
			_unkValue5 = 0;
			return 0;
		}
	}

	if (!(value & _unkValue4)) {
		++_unkValue5;
	}

	dataptr -= 2;
	channel.duration = 1;
	return 2;
}

int AdlibDriver::update_setExtraLevel1(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.opExtraLevel1 = value;
	adjustVolume(channel);
	return 0;
}

int AdlibDriver::updateCallback26(uint8 *&dataptr, Channel &channel, uint8 value) {
	setupDuration(value, channel);
	return (value != 0);
}

int AdlibDriver::update_playNote(uint8 *&dataptr, Channel &channel, uint8 value) {
	setupDuration(value, channel);
	noteOn(channel);
	return (value != 0);
}

int AdlibDriver::update_setFractionalNoteSpacing(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.fractionalSpacing = value & 7;
	return 0;
}

int AdlibDriver::update_setTempo(uint8 *&dataptr, Channel &channel, uint8 value) {
	_tempo = (int8)value;
	return 0;
}

int AdlibDriver::update_removeSecondaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value) {
	--dataptr;
	channel.secondaryEffect = 0;
	return 0;
}

int AdlibDriver::update_setChannelTempo(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.tempo = (int8)value;
	return 0;
}

int AdlibDriver::update_setExtraLevel3(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.opExtraLevel3 = value;
	return 0;
}

int AdlibDriver::update_setExtraLevel2(uint8 *&dataptr, Channel &channel, uint8 value) {
	int channelBackUp = _curChannel;

	_curChannel = value;
	Channel &channel2 = _channels[value];
	channel2.opExtraLevel2 = *dataptr++;
	adjustVolume(channel2);

	_curChannel = channelBackUp;
	return 0;
}

int AdlibDriver::update_changeExtraLevel2(uint8 *&dataptr, Channel &channel, uint8 value) {
	int channelBackUp = _curChannel;

	_curChannel = value;
	Channel &channel2 = _channels[value];
	channel2.opExtraLevel2 += *dataptr++;
	adjustVolume(channel2);

	_curChannel = channelBackUp;
	return 0;
}

int AdlibDriver::update_setAMDepth(uint8 *&dataptr, Channel &channel, uint8 value) {
	if (value & 1)
		_unkOutputByte2 |= 0x80;
	else
		_unkOutputByte2 &= 0x7F;

	// The AM Depth bit is set or cleared, the others remain unchanged
	writeOPL(0xBD, _unkOutputByte2);
	return 0;
}

int AdlibDriver::update_setVibratoDepth(uint8 *&dataptr, Channel &channel, uint8 value) {
	if (value & 1)
		_unkOutputByte2 |= 0x40;
	else
		_unkOutputByte2 &= 0xBF;

	// The Vibrato Depth bit is set or cleared, the others remain unchanged
	writeOPL(0xBD, _unkOutputByte2);
	return 0;
}

int AdlibDriver::update_changeExtraLevel1(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.opExtraLevel1 += value;
	adjustVolume(channel);
	return 0;
}

int AdlibDriver::updateCallback38(uint8 *&dataptr, Channel &channel, uint8 value) {
	int channelBackUp = _curChannel;

	_curChannel = value;
	Channel &channel2 = _channels[value];
	channel2.duration = channel2.priority = 0;
	channel2.dataptr = 0;
	channel2.opExtraLevel2 = 0;

	if (value != 9) {
		uint8 outValue = _regOffset[value];

		// Feedback strength / Connection type
		writeOPL(0xC0 + _curChannel, 0x00);

		// Key scaling level / Operator output level
		writeOPL(0x43 + outValue, 0x3F);

		// Sustain Level / Release Rate
		writeOPL(0x83 + outValue, 0xFF);

		// Key On / Octave / Frequency
		writeOPL(0xB0 + _curChannel, 0x00);
	}

	_curChannel = channelBackUp;
	return 0;
}

int AdlibDriver::updateCallback39(uint8 *&dataptr, Channel &channel, uint8 value) {
	uint16 unk = *dataptr++;
	unk |= value << 8;
	unk &= getRandomNr();

	uint16 unk2 = ((channel.regBx & 0x1F) << 8) | channel.regAx;
	unk2 += unk;
	unk2 |= ((channel.regBx & 0x20) << 8);

	// Frequency
	writeOPL(0xA0 + _curChannel, unk2 & 0xFF);

	// Key On / Octave / Frequency
	writeOPL(0xB0 + _curChannel, (unk2 & 0xFF00) >> 8);

	return 0;
}

int AdlibDriver::update_removePrimaryEffect2(uint8 *&dataptr, Channel &channel, uint8 value) {
	--dataptr;
	channel.primaryEffect = 0;
	return 0;
}

int AdlibDriver::updateCallback41(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.unk16 = value;
	setupNote(channel.rawNote, channel, true);
	return 0;
}

int AdlibDriver::update_resetToGlobalTempo(uint8 *&dataptr, Channel &channel, uint8 value) {
	--dataptr;
	channel.tempo = _tempo;
	return 0;
}

int AdlibDriver::update_nop1(uint8 *&dataptr, Channel &channel, uint8 value) {
	--dataptr;
	return 0;
}

int AdlibDriver::updateCallback44(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.unk11 = value;
	return 0;
}

int AdlibDriver::updateCallback45(uint8 *&dataptr, Channel &channel, uint8 value) {
	if (value & 0x80) {
		value += channel.tempo;
		if ((int8)value >= (int8)channel.tempo)
			value = 1;
	} else {
		int8 temp = value;
		value += channel.tempo;
		if (value < temp)
			value = (uint8)-1;
	}
	channel.tempo = (int8)value;
	return 0;
}

int AdlibDriver::updateCallback46(uint8 *&dataptr, Channel &channel, uint8 value) {
	uint8 entry = *dataptr++;
	_tablePtr1 = _unkTable2[entry++];
	_tablePtr2 = _unkTable2[entry];
	if (value == 2) {
		// Frequency
		writeOPL(0xA0, _tablePtr2[0]);
	}
	return 0;
}

// TODO: This is really the same as update_nop1(), so they should be combined
//       into one single update_nop().

int AdlibDriver::update_nop2(uint8 *&dataptr, Channel &channel, uint8 value) {
	--dataptr;
	return 0;
}

int AdlibDriver::updateCallback48(uint8 *&dataptr, Channel &channel, uint8 value) {
	int channelBackUp = _curChannel;
	int regOffsetBackUp = _curRegOffset;

	uint8 entry = value << 1;
	uint8 *ptr = _soundData + READ_LE_UINT16(_soundData + entry + 0x1F4);

	_curChannel = 6;
	_curRegOffset = _regOffset[6];

	_unkValue6 = *(ptr + 6);
	setupInstrument(_curRegOffset, ptr, channel);

	entry = *dataptr++ << 1;
	ptr = _soundData + READ_LE_UINT16(_soundData + entry + 0x1F4);

	_curChannel = 7;
	_curRegOffset = _regOffset[7];

	_unkValue7 = entry = *(ptr + 5);
	_unkValue8 = entry = *(ptr + 6);
	setupInstrument(_curRegOffset, ptr, channel);

	entry = *dataptr++ << 1;
	ptr = _soundData + READ_LE_UINT16(_soundData + entry + 0x1F4);

	_curChannel = 8;
	_curRegOffset = _regOffset[8];

	_unkValue9 = entry = *(ptr + 5);
	_unkValue10 = entry = *(ptr + 6);
	setupInstrument(_curRegOffset, ptr, channel);

	// Octave / F-Number / Key-On for channels 6, 7 and 8

	_channels[6].regBx = *dataptr++ & 0x2F;
	writeOPL(0xB6, _channels[6].regBx);
	writeOPL(0xA6, *dataptr++);

	_channels[7].regBx = *dataptr++ & 0x2F;
	writeOPL(0xB7, _channels[7].regBx);
	writeOPL(0xA7, *dataptr++);

	_channels[8].regBx = *dataptr++ & 0x2F;
	writeOPL(0xB8, _channels[8].regBx);
	writeOPL(0xA8, *dataptr++);

	_unk4 = 0x20;

	_curRegOffset = regOffsetBackUp;
	_curChannel = channelBackUp;
	return 0;
}

int AdlibDriver::updateCallback49(uint8 *&dataptr, Channel &channel, uint8 value) {
	// Amplitude Modulation Depth / Vibrato Depth / Rhythm
	writeOPL(0xBD, (((value & 0x1F) ^ 0xFF) & _unk4) | 0x20);

	value |= _unk4;
	_unk4 = value;

	value |= _unkOutputByte2;
	value |= 0x20;

	// FIXME: This could probably be replaced with writeOPL(0xBD, value),
	//        but to make it easier to compare the output to DOSbox, write
	//        directly to the data port and do the probably unnecessary
	//        delay loop.

	OPLWrite(_adlib, 0x389, value);

	for (int i = 0; i < 23;  i++)
		OPLRead(_adlib, 0x388);

	return 0;
}

int AdlibDriver::updateCallback50(uint8 *&dataptr, Channel &channel, uint8 value) {
	--dataptr;
	_unk4 = 0;

	// Amplitude Modulation Depth / Vibrato Depth / Rhythm
	writeOPL(0xBD, _unkOutputByte2 & 0xC0);

	return 0;
}

int AdlibDriver::updateCallback51(uint8 *&dataptr, Channel &channel, uint8 value) {
	uint16 temp = (value << 8) | *dataptr++;

	if (value & 1) {
		uint8 val = temp & 0xFF;
		_unkValue12 = val;
		val += _unkValue7;
		val += _unkValue11;
		val += _unkValue12;

		// Channel 2: Level Key Scaling / Total Level
		writeOPL(0x51, checkValue(val));
	}

	if (value & 2) {
		uint8 val = temp & 0xFF;
		_unkValue14 = val;
		val += _unkValue10;
		val += _unkValue13;
		val += _unkValue14;

		// Channel 3: Level Key Scaling / Total Level
		writeOPL(0x55, checkValue(val));
	}

	if (value & 4) {
		uint8 val = temp & 0xFF;
		_unkValue15 = val;
		val += _unkValue9;
		val += _unkValue16;
		val += _unkValue15;

		// Channel 3: Level Key Scaling / Total Level
		writeOPL(0x52, checkValue(val));
	}

	if (value & 8) {
		uint8 val = temp & 0xFF;
		_unkValue18 = val;
		val += _unkValue8;
		val += _unkValue17;
		val += _unkValue18;

		// Channel 2: Level Key Scaling / Total Level
		writeOPL(0x54, checkValue(val));
	}

	if (value & 16) {
		uint8 val = temp & 0xFF;
		_unkValue20 = val;
		val += _unkValue6;
		val += _unkValue19;
		val += _unkValue20;

		// Channel 1: Level Key Scaling / Total Level
		writeOPL(0x53, checkValue(val));
	}

	return 0;
}

int AdlibDriver::updateCallback52(uint8 *&dataptr, Channel &channel, uint8 value) {
	uint16 temp = (value << 8) | *dataptr++;

	if (value & 1) {
		uint8 val = temp & 0xFF;
		val += _unkValue7;
		val += _unkValue11;
		val += _unkValue12;

		// Channel 2: Level Key Scaling / Total Level
		writeOPL(0x51, checkValue(val));
	}

	if (value & 2) {
		uint8 val = temp & 0xFF;
		val += _unkValue10;
		val += _unkValue13;
		val += _unkValue14;

		// Channel 3: Level Key Scaling / Total Level
		writeOPL(0x55, checkValue(val));
	}

	if (value & 4) {
		uint8 val = temp & 0xFF;
		val += _unkValue9;
		val += _unkValue16;
		val += _unkValue15;

		// Channel 3: Level Key Scaling / Total Level
		writeOPL(0x52, checkValue(val));
	}

	if (value & 8) {
		uint8 val = temp & 0xFF;
		val += _unkValue8;
		val += _unkValue17;
		val += _unkValue18;

		// Channel 2: Level Key Scaling / Total Level
		writeOPL(0x54, checkValue(val));
	}

	if (value & 16) {
		uint8 val = temp & 0xFF;
		val += _unkValue6;
		val += _unkValue19;
		val += _unkValue20;

		// Channel 1: Level Key Scaling / Total Level
		writeOPL(0x53, checkValue(val));
	}

	return 0;
}

int AdlibDriver::updateCallback53(uint8 *&dataptr, Channel &channel, uint8 value) {
	uint16 temp = (value << 8) | *dataptr++;

	if (value & 1) {
		uint8 val = temp & 0xFF;
		_unkValue11 = val;
		val += _unkValue7;
		val += _unkValue12;

		// Channel 2: Level Key Scaling / Total Level
		writeOPL(0x51, checkValue(val));
	}

	if (value & 2) {
		uint8 val = temp & 0xFF;
		_unkValue13 = val;
		val += _unkValue10;
		val += _unkValue14;

		// Channel 3: Level Key Scaling / Total Level
		writeOPL(0x55, checkValue(val));
	}

	if (value & 4) {
		uint8 val = temp & 0xFF;
		_unkValue16 = val;
		val += _unkValue9;
		val += _unkValue15;

		// Channel 3: Level Key Scaling / Total Level
		writeOPL(0x52, checkValue(val));
	}

	if (value & 8) {
		uint8 val = temp & 0xFF;
		_unkValue17 = val;
		val += _unkValue8;
		val += _unkValue18;

		// Channel 2: Level Key Scaling / Total Level
		writeOPL(0x54, checkValue(val));
	}

	if (value & 16) {
		uint8 val = temp & 0xFF;
		_unkValue19 = val;
		val += _unkValue6;
		val += _unkValue20;

		// Channel 1: Level Key Scaling / Total Level
		writeOPL(0x53, checkValue(val));
	}

	return 0;
}

int AdlibDriver::updateCallback54(uint8 *&dataptr, Channel &channel, uint8 value) {
	_unk5 = value;
	return 0;
}

int AdlibDriver::update_setTempoReset(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.tempoReset = value;
	return 0;
}

int AdlibDriver::updateCallback56(uint8 *&dataptr, Channel &channel, uint8 value) {
	channel.unk39 = value;
	channel.unk40 = *dataptr++;
	return 0;
}

// static res

#define COMMAND(x) { &AdlibDriver::x, #x }
const AdlibDriver::OpcodeEntry AdlibDriver::_opcodeList[] = {
	COMMAND(snd_ret0x100),
	COMMAND(snd_ret0x1983),
	COMMAND(snd_initDriver),
	COMMAND(snd_deinitDriver),
	COMMAND(snd_setSoundData),
	COMMAND(snd_unkOpcode1),
	COMMAND(snd_startSong),
	COMMAND(snd_unkOpcode2),
	COMMAND(snd_unkOpcode3),
	COMMAND(snd_readByte),
	COMMAND(snd_writeByte),
	COMMAND(snd_setUnk5),
	COMMAND(snd_unkOpcode4),
	COMMAND(snd_dummy),
	COMMAND(snd_getNullvar4),
	COMMAND(snd_setNullvar3),
	COMMAND(snd_setFlag),
	COMMAND(snd_clearFlag)
};

const AdlibDriver::ParserOpcode AdlibDriver::_parserOpcodeTable[] = {
	// 0
	COMMAND(update_setRepeat),
	COMMAND(update_checkRepeat),
	COMMAND(updateCallback3),
	COMMAND(update_setNoteSpacing),

	// 4
	COMMAND(update_jump),
	COMMAND(update_jumpToSubroutine),
	COMMAND(update_returnFromSubroutine),
	COMMAND(update_setBaseOctave),

	// 8
	COMMAND(update_stopChannel),
	COMMAND(update_playRest),
	COMMAND(update_writeAdlib),
	COMMAND(updateCallback12),

	// 12
	COMMAND(update_setBaseNote),
	COMMAND(update_setupSecondaryEffect1),
	COMMAND(update_stopOtherChannel),
	COMMAND(updateCallback16),

	// 16
	COMMAND(update_setupInstrument),
	COMMAND(update_setupPrimaryEffect1),
	COMMAND(update_removePrimaryEffect1),
	COMMAND(update_setBaseFreq),

	// 20
	COMMAND(update_stopChannel),
	COMMAND(update_setupPrimaryEffect2),
	COMMAND(update_stopChannel),
	COMMAND(update_stopChannel),

	// 24
	COMMAND(update_stopChannel),
	COMMAND(update_stopChannel),
	COMMAND(update_setPriority),
	COMMAND(update_stopChannel),

	// 28
	COMMAND(updateCallback23),
	COMMAND(updateCallback24),
	COMMAND(update_setExtraLevel1),
	COMMAND(update_stopChannel),

	// 32
	COMMAND(updateCallback26),
	COMMAND(update_playNote),
	COMMAND(update_stopChannel),
	COMMAND(update_stopChannel),

	// 36
	COMMAND(update_setFractionalNoteSpacing),
	COMMAND(update_stopChannel),
	COMMAND(update_setTempo),
	COMMAND(update_removeSecondaryEffect1),

	// 40
	COMMAND(update_stopChannel),
	COMMAND(update_setChannelTempo),
	COMMAND(update_stopChannel),
	COMMAND(update_setExtraLevel3),

	// 44
	COMMAND(update_setExtraLevel2),
	COMMAND(update_changeExtraLevel2),
	COMMAND(update_setAMDepth),
	COMMAND(update_setVibratoDepth),

	// 48
	COMMAND(update_changeExtraLevel1),
	COMMAND(update_stopChannel),
	COMMAND(update_stopChannel),
	COMMAND(updateCallback38),

	// 52
	COMMAND(update_stopChannel),
	COMMAND(updateCallback39),
	COMMAND(update_removePrimaryEffect2),
	COMMAND(update_stopChannel),

	// 56
	COMMAND(update_stopChannel),
	COMMAND(updateCallback41),
	COMMAND(update_resetToGlobalTempo),
	COMMAND(update_nop1),

	// 60
	COMMAND(updateCallback44),
	COMMAND(updateCallback45),
	COMMAND(update_stopChannel),
	COMMAND(updateCallback46),

	// 64
	COMMAND(update_nop2),
	COMMAND(updateCallback48),
	COMMAND(updateCallback49),
	COMMAND(updateCallback50),

	// 68
	COMMAND(updateCallback51),
	COMMAND(updateCallback52),
	COMMAND(updateCallback53),
	COMMAND(updateCallback54),

	// 72
	COMMAND(update_setTempoReset),
	COMMAND(updateCallback56),
	COMMAND(update_stopChannel)
};
#undef COMMAND

const int AdlibDriver::_opcodesEntries = ARRAYSIZE(AdlibDriver::_opcodeList);
const int AdlibDriver::_parserOpcodeTableSize = ARRAYSIZE(AdlibDriver::_parserOpcodeTable);

// This table holds the register offset for operator 1 for each of the nine
// channels. To get the register offset for operator 2, simply add 3.

const uint8 AdlibDriver::_regOffset[] = {
	0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11,
	0x12
};

// Given the size of this table, and the range of its values, it's probably the
// F-Numbers (10 bits) for the notes of the 12-tone scale. However, it does not
// match the table in the Adlib documentation I've seen.

const uint16 AdlibDriver::_unkTable[] = {
	0x0134, 0x0147, 0x015A, 0x016F, 0x0184, 0x019C, 0x01B4, 0x01CE, 0x01E9,
	0x0207, 0x0225, 0x0246
};

// These tables are currently only used by updateCallback46(), which only ever
// uses the first element of one of the sub-tables.

const uint8 *AdlibDriver::_unkTable2[] = {
	AdlibDriver::_unkTable2_1,
	AdlibDriver::_unkTable2_2,
	AdlibDriver::_unkTable2_1,
	AdlibDriver::_unkTable2_2,
	AdlibDriver::_unkTable2_3,
	AdlibDriver::_unkTable2_2
};

const uint8 AdlibDriver::_unkTable2_1[] = {
	0x50, 0x50, 0x4F, 0x4F, 0x4E, 0x4E, 0x4D, 0x4D,
	0x4C, 0x4C, 0x4B, 0x4B, 0x4A, 0x4A, 0x49, 0x49,
	0x48, 0x48, 0x47, 0x47, 0x46, 0x46, 0x45, 0x45,
	0x44, 0x44, 0x43, 0x43, 0x42, 0x42, 0x41, 0x41,
	0x40, 0x40, 0x3F, 0x3F, 0x3E, 0x3E, 0x3D, 0x3D,
	0x3C, 0x3C, 0x3B, 0x3B, 0x3A, 0x3A, 0x39, 0x39,
	0x38, 0x38, 0x37, 0x37, 0x36, 0x36, 0x35, 0x35,
	0x34, 0x34, 0x33, 0x33, 0x32, 0x32, 0x31, 0x31,
	0x30, 0x30, 0x2F, 0x2F, 0x2E, 0x2E, 0x2D, 0x2D,
	0x2C, 0x2C, 0x2B, 0x2B, 0x2A, 0x2A, 0x29, 0x29,
	0x28, 0x28, 0x27, 0x27, 0x26, 0x26, 0x25, 0x25,
	0x24, 0x24, 0x23, 0x23, 0x22, 0x22, 0x21, 0x21,
	0x20, 0x20, 0x1F, 0x1F, 0x1E, 0x1E, 0x1D, 0x1D,
	0x1C, 0x1C, 0x1B, 0x1B, 0x1A, 0x1A, 0x19, 0x19,
	0x18, 0x18, 0x17, 0x17, 0x16, 0x16, 0x15, 0x15,
	0x14, 0x14, 0x13, 0x13, 0x12, 0x12, 0x11, 0x11,
	0x10, 0x10
};

// no don't ask me WHY this table exsits!
const uint8 AdlibDriver::_unkTable2_2[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x6F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F
};

const uint8 AdlibDriver::_unkTable2_3[] = {
	0x40, 0x40, 0x40, 0x3F, 0x3F, 0x3F, 0x3E, 0x3E,
	0x3E, 0x3D, 0x3D, 0x3D, 0x3C, 0x3C, 0x3C, 0x3B,
	0x3B, 0x3B, 0x3A, 0x3A, 0x3A, 0x39, 0x39, 0x39,
	0x38, 0x38, 0x38, 0x37, 0x37, 0x37, 0x36, 0x36,
	0x36, 0x35, 0x35, 0x35, 0x34, 0x34, 0x34, 0x33,
	0x33, 0x33, 0x32, 0x32, 0x32, 0x31, 0x31, 0x31,
	0x30, 0x30, 0x30, 0x2F, 0x2F, 0x2F, 0x2E, 0x2E,
	0x2E, 0x2D, 0x2D, 0x2D, 0x2C, 0x2C, 0x2C, 0x2B,
	0x2B, 0x2B, 0x2A, 0x2A, 0x2A, 0x29, 0x29, 0x29,
	0x28, 0x28, 0x28, 0x27, 0x27, 0x27, 0x26, 0x26,
	0x26, 0x25, 0x25, 0x25, 0x24, 0x24, 0x24, 0x23,
	0x23, 0x23, 0x22, 0x22, 0x22, 0x21, 0x21, 0x21,
	0x20, 0x20, 0x20, 0x1F, 0x1F, 0x1F, 0x1E, 0x1E,
	0x1E, 0x1D, 0x1D, 0x1D, 0x1C, 0x1C, 0x1C, 0x1B,
	0x1B, 0x1B, 0x1A, 0x1A, 0x1A, 0x19, 0x19, 0x19,
	0x18, 0x18, 0x18, 0x17, 0x17, 0x17, 0x16, 0x16,
	0x16, 0x15
};

// This table is used to modify the frequency of the notes, depending on the
// note value and unk16. In theory, we could very well try to access memory
// outside this table, but in reality that probably won't happen.
//
// This could be some sort of pitch bend, but I have yet to see it used for
// anything so it's hard to say.

// TODO: format this
const uint8 AdlibDriver::_unkTables[][32] = {
	{    0x00,  0x01,  0x02,  0x03,  0x04,  0x05,  0x06,  0x08,
	     0x09,  0x0A,  0x0B,  0x0C,  0x0D,  0x0E,  0x0F,  0x10,
	     0x11,  0x12,  0x13,  0x14,  0x15,  0x16,  0x17,  0x19,
	     0x1A,  0x1B,  0x1C,  0x1D,  0x1E,  0x1F,  0x20,  0x21 },
	{    0x00,  0x01,  0x02,  0x03,  0x04,  0x06,  0x07,  0x09,
	     0x0A,  0x0B,  0x0C,  0x0D,  0x0E,  0x0F,  0x10,  0x11,
	     0x12,  0x13,  0x14,  0x15,  0x16,  0x17,  0x18,  0x1A,
	     0x1B,  0x1C,  0x1D,  0x1E,  0x1F,  0x20,  0x22,  0x24 },
	{    0x00,  0x01,  0x02,  0x03,  0x04,  0x06,  0x08,  0x09,
	     0x0A,  0x0C,  0x0D,  0x0E,  0x0F,  0x11,  0x12,  0x13,
	     0x14,  0x15,  0x16,  0x17,  0x19,  0x1A,  0x1C,  0x1D,
	     0x1E,  0x1F,  0x20,  0x21,  0x22,  0x24,  0x25,  0x26 },
	{    0x00,  0x01,  0x02,  0x03,  0x04,  0x06,  0x08,  0x0A,
	     0x0B,  0x0C,  0x0D,  0x0E,  0x0F,  0x11,  0x12,  0x13,
	     0x14,  0x15,  0x16,  0x17,  0x18,  0x1A,  0x1C,  0x1D,
	     0x1E,  0x1F,  0x20,  0x21,  0x23,  0x25,  0x27,  0x28 },
	{    0x00,  0x01,  0x02,  0x03,  0x04,  0x06,  0x08,  0x0A,
	     0x0B,  0x0C,  0x0D,  0x0E,  0x0F,  0x11,  0x13,  0x15,
	     0x16,  0x17,  0x18,  0x19,  0x1B,  0x1D,  0x1F,  0x20,
	     0x21,  0x22,  0x23,  0x24,  0x25,  0x26,  0x28,  0x2A },
	{    0x00,  0x01,  0x02,  0x03,  0x05,  0x07,  0x09,  0x0B,
	     0x0C,  0x0D,  0x0E,  0x0F,  0x10,  0x11,  0x13,  0x15,
	     0x16,  0x17,  0x18,  0x19,  0x1B,  0x1D,  0x1F,  0x20,
	     0x21,  0x22,  0x23,  0x24,  0x25,  0x26,  0x28,  0x2A },
	{    0x00,  0x01,  0x02,  0x03,  0x05,  0x07,  0x09,  0x0B,
	     0x0C,  0x0D,  0x0E,  0x0F,  0x10,  0x11,  0x13,  0x15,
	     0x16,  0x17,  0x18,  0x19,  0x1B,  0x1D,  0x1F,  0x20,
	     0x21,  0x22,  0x23,  0x25,  0x27,  0x29,  0x2B,  0x2D },
	{    0x00,  0x01,  0x02,  0x03,  0x05,  0x07,  0x09,  0x0B,
	     0x0C,  0x0D,  0x0E,  0x0F,  0x10,  0x11,  0x13,  0x15,
	     0x16,  0x17,  0x18,  0x1A,  0x1C,  0x1E,  0x21,  0x24,
	     0x25,  0x26,  0x27,  0x29,  0x2B,  0x2D,  0x2F,  0x30 },
	{    0x00,  0x01,  0x02,  0x04,  0x06,  0x08,  0x0A,  0x0C,
	     0x0D,  0x0E,  0x0F,  0x10,  0x11,  0x13,  0x15,  0x18,
	     0x19,  0x1A,  0x1C,  0x1D,  0x1F,  0x21,  0x23,  0x25,
	     0x26,  0x27,  0x29,  0x2B,  0x2D,  0x2F,  0x30,  0x32 },
	{    0x00,  0x01,  0x02,  0x04,  0x06,  0x08,  0x0A,  0x0D,
	     0x0E,  0x0F,  0x10,  0x11,  0x12,  0x14,  0x17,  0x1A,
	     0x19,  0x1A,  0x1C,  0x1E,  0x20,  0x22,  0x25,  0x28,
	     0x29,  0x2A,  0x2B,  0x2D,  0x2F,  0x31,  0x33,  0x35 },
	{    0x00,  0x01,  0x03,  0x05,  0x07,  0x09,  0x0B,  0x0E,
	     0x0F,  0x10,  0x12,  0x14,  0x16,  0x18,  0x1A,  0x1B,
	     0x1C,  0x1D,  0x1E,  0x20,  0x22,  0x24,  0x26,  0x29,
	     0x2A,  0x2C,  0x2E,  0x30,  0x32,  0x34,  0x36,  0x39 },
	{    0x00,  0x01,  0x03,  0x05,  0x07,  0x09,  0x0B,  0x0E,
	     0x0F,  0x10,  0x12,  0x14,  0x16,  0x19,  0x1B,  0x1E,
	     0x1F,  0x21,  0x23,  0x25,  0x27,  0x29,  0x2B,  0x2D,
	     0x2E,  0x2F,  0x31,  0x32,  0x34,  0x36,  0x39,  0x3C },
	{    0x00,  0x01,  0x03,  0x05,  0x07,  0x0A,  0x0C,  0x0F,
	     0x10,  0x11,  0x13,  0x15,  0x17,  0x19,  0x1B,  0x1E,
	     0x1F,  0x20,  0x22,  0x24,  0x26,  0x28,  0x2B,  0x2E,
	     0x2F,  0x30,  0x32,  0x34,  0x36,  0x39,  0x3C,  0x3F },
	{    0x00,  0x02,  0x04,  0x06,  0x08,  0x0B,  0x0D,  0x10,
	     0x11,  0x12,  0x14,  0x16,  0x18,  0x1B,  0x1E,  0x21,
	     0x22,  0x23,  0x25,  0x27,  0x29,  0x2C,  0x2F,  0x32,
	     0x33,  0x34,  0x36,  0x38,  0x3B,  0x34,  0x41,  0x44 },
	{    0x00,  0x02,  0x04,  0x06,  0x08,  0x0B,  0x0D,  0x11,
	     0x12,  0x13,  0x15,  0x17,  0x1A,  0x1D,  0x20,  0x23,
	     0x24,  0x25,  0x27,  0x29,  0x2C,  0x2F,  0x32,  0x35,
	     0x36,  0x37,  0x39,  0x3B,  0x3E,  0x41,  0x44,  0x47 }
};

#pragma mark -

SoundAdlibPC::SoundAdlibPC(Audio::Mixer *mixer, KyraEngine *engine)
	: Sound(engine, mixer), _driver(0), _trackEntries(), _soundDataPtr(0) {
	memset(_trackEntries, 0, sizeof(_trackEntries));
	_driver = new AdlibDriver(mixer);
	assert(_driver);

	_sfxPlayingSound = -1;
	_soundFileLoaded = "";
}

SoundAdlibPC::~SoundAdlibPC() {
	delete [] _soundDataPtr;
	delete _driver;
}

bool SoundAdlibPC::init() {
	_driver->callback(2);
	_driver->callback(16, int(4));
	return true;
}

void SoundAdlibPC::setVolume(int volume) {
}

int SoundAdlibPC::getVolume() {
	return 0;
}

void SoundAdlibPC::loadMusicFile(const char *file) {
	loadSoundFile(file);
}

void SoundAdlibPC::playTrack(uint8 track) {
	playSoundEffect(track);
}

void SoundAdlibPC::haltTrack() {
	unk1();
	unk2();
	//_engine->_system->delayMillis(3 * 60);
}

void SoundAdlibPC::playSoundEffect(uint8 track) {
	uint8 soundId = _trackEntries[track];
	if ((int8)soundId == -1 || !_soundDataPtr)
		return;
	soundId &= 0xFF;
	while ((_driver->callback(16, 0) & 8)) {
		// We call the system delay and not the game delay to avoid concurrency issues.
		_engine->_system->delayMillis(10);
	}
	if (_sfxPlayingSound != -1) {
		_driver->callback(10, _sfxPlayingSound, int(1), int(_sfxSecondByteOfSong));
		_driver->callback(10, _sfxPlayingSound, int(3), int(_sfxFourthByteOfSong));
		_sfxPlayingSound = -1;
	}

	int firstByteOfSong = _driver->callback(9, soundId, int(0));

	if (firstByteOfSong != 9) {
		_sfxPlayingSound = soundId;
		_sfxSecondByteOfSong = _driver->callback(9, soundId, int(1));
		_sfxFourthByteOfSong = _driver->callback(9, soundId, int(3));

		int newVal = ((((-_sfxFourthByteOfSong) + 63) * 0xFF) >> 8) & 0xFF;
		newVal = -newVal + 63;
		_driver->callback(10, soundId, int(3), newVal);
		newVal = ((_sfxSecondByteOfSong * 0xFF) >> 8) & 0xFF;
		_driver->callback(10, soundId, int(1), newVal);
	}

	_driver->callback(6, soundId);
}

void SoundAdlibPC::beginFadeOut() {
	playSoundEffect(1);
}

void SoundAdlibPC::loadSoundFile(const char *file) {
	if (_soundFileLoaded == file)
		return;

	if (_soundDataPtr) {
		haltTrack();
	}

	uint8 *file_data = 0; uint32 file_size = 0;

	char filename[25];
	sprintf(filename, "%s.ADL", file);

	file_data = _engine->resource()->fileData(filename, &file_size);
	if (!file_data) {
		warning("Couldn't find music file: '%s'", filename);
		return;
	}

	unk2();
	unk1();

	_driver->callback(8, int(-1));
	_soundDataPtr = 0;

	uint8 *p = file_data;
	memcpy(_trackEntries, p, 120*sizeof(uint8));
	p += 120;

	int soundDataSize = file_size - 120;

	_soundDataPtr = new uint8[soundDataSize];
	assert(_soundDataPtr);

	memcpy(_soundDataPtr, p, soundDataSize*sizeof(uint8));

	delete [] file_data;
	file_data = p = 0;
	file_size = 0;

	_driver->callback(4, _soundDataPtr);

	_soundFileLoaded = file;
}

void SoundAdlibPC::unk1() {
	playSoundEffect(0);
	//_engine->_system->delayMillis(5 * 60);
}

void SoundAdlibPC::unk2() {
	playSoundEffect(0);
}

} // end of namespace Kyra

