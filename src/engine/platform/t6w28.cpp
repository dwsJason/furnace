/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2022 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "t6w28.h"
#include "../engine.h"
#include "sound/t6w28/T6W28_Apu.h"
#include <math.h>

//#define rWrite(a,v) pendingWrites[a]=v;
#define rWrite(a,v) if (!skipRegisterWrites) {writes.emplace(a,v); if (dumpWrites) {addWrite(a,v);} }

#define CHIP_DIVIDER 16

const char* regCheatSheetT6W28[]={
  "Data0", "0",
  "Data1", "1",
  NULL
};

const char** DivPlatformT6W28::getRegisterSheet() {
  return regCheatSheetT6W28;
}

void DivPlatformT6W28::acquire(short* bufL, short* bufR, size_t start, size_t len) {
  for (size_t h=start; h<start+len; h++) {
    cycles=0;
    while (!writes.empty() && cycles<16) {
      QueuedWrite w=writes.front();
      if (w.addr) {
        t6w->write_data_right(cycles,w.val);
      } else {
        t6w->write_data_left(cycles,w.val);
      }
      regPool[w.addr&1]=w.val;
      //cycles+=2;
      writes.pop();
    }
    t6w->end_frame(16);

    tempL=0;
    tempR=0;
    for (int i=0; i<4; i++) {
      oscBuf[i]->data[oscBuf[i]->needle++]=(out[i][1].curValue+out[i][2].curValue)<<6;
      tempL+=out[i][1].curValue<<7;
      tempR+=out[i][2].curValue<<7;
    }

    if (tempL<-32768) tempL=-32768;
    if (tempL>32767) tempL=32767;
    if (tempR<-32768) tempR=-32768;
    if (tempR>32767) tempR=32767;
    
    bufL[h]=tempL;
    bufR[h]=tempR;
  }
}

void DivPlatformT6W28::writeOutVol(int ch) {
  int left=15-CLAMP(chan[ch].outVol+chan[ch].panL-15,0,15);
  int right=15-CLAMP(chan[ch].outVol+chan[ch].panR-15,0,15);
  rWrite(0,0x90|(ch<<5)|(isMuted[ch]?15:left));
  rWrite(1,0x90|(ch<<5)|(isMuted[ch]?15:right));
}

void DivPlatformT6W28::tick(bool sysTick) {
  for (int i=0; i<4; i++) {
    chan[i].std.next();
    if (chan[i].std.vol.had) {
      chan[i].outVol=VOL_SCALE_LOG(chan[i].vol&15,MIN(15,chan[i].std.vol.val),15);
    }
    if (chan[i].std.arp.had) {
      if (!chan[i].inPorta) {
        int noiseSeek=parent->calcArp(chan[i].note,chan[i].std.arp.val);
        chan[i].baseFreq=NOTE_PERIODIC(noiseSeek);
      }
      chan[i].freqChanged=true;
    }
    if (chan[i].std.panL.had) {
      chan[i].panL=chan[i].std.panL.val&15;
    }
    if (chan[i].std.panR.had) {
      chan[i].panR=chan[i].std.panR.val&15;
    }
    if (chan[i].std.vol.had || chan[i].std.panL.had || chan[i].std.panR.had) {
      writeOutVol(i);
    }
    if (chan[i].std.pitch.had) {
      if (chan[i].std.pitch.mode) {
        chan[i].pitch2+=chan[i].std.pitch.val;
        CLAMP_VAR(chan[i].pitch2,-32768,32767);
      } else {
        chan[i].pitch2=chan[i].std.pitch.val;
      }
      chan[i].freqChanged=true;
    }
    if (chan[i].freqChanged || chan[i].keyOn || chan[i].keyOff) {
      //DivInstrument* ins=parent->getIns(chan[i].ins,DIV_INS_PCE);
      chan[i].freq=parent->calcFreq(chan[i].baseFreq,chan[i].pitch,true,0,chan[i].pitch2,chipClock,CHIP_DIVIDER);
      if (chan[i].freq>1023) chan[i].freq=1023;
      if (i==3) {
        rWrite(1,0xe7);
        rWrite(1,0x80|(2<<5)|(chan[3].freq&15));
        rWrite(1,chan[3].freq>>4);
      } else {
        rWrite(0,0x80|i<<5|(chan[i].freq&15));
        rWrite(0,chan[i].freq>>4);
      }
      if (chan[i].keyOn) chan[i].keyOn=false;
      if (chan[i].keyOff) chan[i].keyOff=false;
      chan[i].freqChanged=false;
    }
  }
}

int DivPlatformT6W28::dispatch(DivCommand c) {
  switch (c.cmd) {
    case DIV_CMD_NOTE_ON: {
      DivInstrument* ins=parent->getIns(chan[c.chan].ins,DIV_INS_PCE);
      if (c.value!=DIV_NOTE_NULL) {
        chan[c.chan].baseFreq=NOTE_PERIODIC(c.value);
        chan[c.chan].freqChanged=true;
        chan[c.chan].note=c.value;
      }
      chan[c.chan].active=true;
      chan[c.chan].keyOn=true;
      chan[c.chan].macroInit(ins);
      if (!parent->song.brokenOutVol && !chan[c.chan].std.vol.will) {
        chan[c.chan].outVol=chan[c.chan].vol;
      }
      chan[c.chan].insChanged=false;
      break;
    }
    case DIV_CMD_NOTE_OFF:
      chan[c.chan].active=false;
      chan[c.chan].keyOff=true;
      chan[c.chan].macroInit(NULL);
      break;
    case DIV_CMD_NOTE_OFF_ENV:
    case DIV_CMD_ENV_RELEASE:
      chan[c.chan].std.release();
      break;
    case DIV_CMD_INSTRUMENT:
      if (chan[c.chan].ins!=c.value || c.value2==1) {
        chan[c.chan].ins=c.value;
        chan[c.chan].insChanged=true;
      }
      break;
    case DIV_CMD_VOLUME:
      if (chan[c.chan].vol!=c.value) {
        chan[c.chan].vol=c.value;
        if (!chan[c.chan].std.vol.has) {
          chan[c.chan].outVol=c.value;
          if (chan[c.chan].active) {
          }
        }
      }
      break;
    case DIV_CMD_GET_VOLUME:
      if (chan[c.chan].std.vol.has) {
        return chan[c.chan].vol;
      }
      return chan[c.chan].outVol;
      break;
    case DIV_CMD_PITCH:
      chan[c.chan].pitch=c.value;
      chan[c.chan].freqChanged=true;
      break;
    case DIV_CMD_NOTE_PORTA: {
      int destFreq=NOTE_PERIODIC(c.value2);
      bool return2=false;
      if (destFreq>chan[c.chan].baseFreq) {
        chan[c.chan].baseFreq+=c.value;
        if (chan[c.chan].baseFreq>=destFreq) {
          chan[c.chan].baseFreq=destFreq;
          return2=true;
        }
      } else {
        chan[c.chan].baseFreq-=c.value;
        if (chan[c.chan].baseFreq<=destFreq) {
          chan[c.chan].baseFreq=destFreq;
          return2=true;
        }
      }
      chan[c.chan].freqChanged=true;
      if (return2) {
        chan[c.chan].inPorta=false;
        return 2;
      }
      break;
    }
    case DIV_CMD_STD_NOISE_MODE:
      chan[c.chan].noise=c.value;
      break;
    case DIV_CMD_PANNING: {
      chan[c.chan].panL=c.value>>4;
      chan[c.chan].panR=c.value2>>4;
      writeOutVol(c.chan);
      break;
    }
    case DIV_CMD_LEGATO:
      chan[c.chan].baseFreq=NOTE_PERIODIC(c.value+((chan[c.chan].std.arp.will && !chan[c.chan].std.arp.mode)?(chan[c.chan].std.arp.val):(0)));
      chan[c.chan].freqChanged=true;
      chan[c.chan].note=c.value;
      break;
    case DIV_CMD_PRE_PORTA:
      if (chan[c.chan].active && c.value2) {
        if (parent->song.resetMacroOnPorta) chan[c.chan].macroInit(parent->getIns(chan[c.chan].ins,DIV_INS_PCE));
      }
      if (!chan[c.chan].inPorta && c.value && !parent->song.brokenPortaArp && chan[c.chan].std.arp.will) chan[c.chan].baseFreq=NOTE_PERIODIC(chan[c.chan].note);
      chan[c.chan].inPorta=c.value;
      break;
    case DIV_CMD_GET_VOLMAX:
      return 31;
      break;
    case DIV_ALWAYS_SET_VOLUME:
      return 1;
      break;
    default:
      break;
  }
  return 1;
}

void DivPlatformT6W28::muteChannel(int ch, bool mute) {
  isMuted[ch]=mute;
}

void DivPlatformT6W28::forceIns() {
  for (int i=0; i<4; i++) {
    chan[i].insChanged=true;
    chan[i].freqChanged=true;
  }
}

void* DivPlatformT6W28::getChanState(int ch) {
  return &chan[ch];
}

DivMacroInt* DivPlatformT6W28::getChanMacroInt(int ch) {
  return &chan[ch].std;
}

DivDispatchOscBuffer* DivPlatformT6W28::getOscBuffer(int ch) {
  return oscBuf[ch];
}

unsigned char* DivPlatformT6W28::getRegisterPool() {
  return regPool;
}

int DivPlatformT6W28::getRegisterPoolSize() {
  return 112;
}

void DivPlatformT6W28::reset() {
  while (!writes.empty()) writes.pop();
  memset(regPool,0,128);
  for (int i=0; i<4; i++) {
    chan[i]=DivPlatformT6W28::Channel();
    chan[i].std.setEngine(parent);

    out[i][0].curValue=0;
    out[i][1].curValue=0;
    out[i][2].curValue=0;
  }
  if (dumpWrites) {
    addWrite(0xffffffff,0);
  }
  t6w->reset();
  lastPan=0xff;
  tempL=0;
  tempR=0;
  cycles=0;
  curChan=-1;
  delay=0;
}

bool DivPlatformT6W28::isStereo() {
  return true;
}

bool DivPlatformT6W28::keyOffAffectsArp(int ch) {
  return true;
}

void DivPlatformT6W28::notifyInsDeletion(void* ins) {
  for (int i=0; i<4; i++) {
    chan[i].std.notifyInsDeletion((DivInstrument*)ins);
  }
}

void DivPlatformT6W28::setFlags(const DivConfig& flags) {
  chipClock=3072000.0;
  rate=chipClock/16;
  for (int i=0; i<4; i++) {
    oscBuf[i]->rate=rate;
  }

  if (t6w!=NULL) {
    delete t6w;
    t6w=NULL;
  }
  t6w=new MDFN_IEN_NGP::T6W28_Apu;
  for (int i=0; i<4; i++) {
    t6w->osc_output(i,&out[i][0],&out[i][1],&out[i][2]);
  }
}

void DivPlatformT6W28::poke(unsigned int addr, unsigned short val) {
  rWrite(addr,val);
}

void DivPlatformT6W28::poke(std::vector<DivRegWrite>& wlist) {
  for (DivRegWrite& i: wlist) rWrite(i.addr,i.val);
}

int DivPlatformT6W28::init(DivEngine* p, int channels, int sugRate, const DivConfig& flags) {
  parent=p;
  dumpWrites=false;
  skipRegisterWrites=false;
  for (int i=0; i<4; i++) {
    isMuted[i]=false;
    oscBuf[i]=new DivDispatchOscBuffer;
  }
  t6w=NULL;
  setFlags(flags);
  reset();
  return 6;
}

void DivPlatformT6W28::quit() {
  for (int i=0; i<4; i++) {
    delete oscBuf[i];
  }
  if (t6w!=NULL) {
    delete t6w;
    t6w=NULL;
  }
}

DivPlatformT6W28::~DivPlatformT6W28() {
}