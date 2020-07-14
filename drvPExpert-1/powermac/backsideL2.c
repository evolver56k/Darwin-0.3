/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 *  backsizeL2.c - Support for PowerPC 750 L2 Cache
 *
 *  (c) Apple Computer, Inc. 1998
 *
 *  Writen by: Josh de Cesare
 *
 */


#include <proc_reg.h>
#include <powermac.h>


// Constants from PowerPC 750 Book Table 9-1
enum {
	kL2IPShift	=  0,
	kL2BYPShift	= 13,
	kL2DFShift	= 14,
	kL2SLShift	= 15,
	kL2OHShift	= 16,
	kL2TSShift	= 18,
	kL2WTShift	= 19,
	kL2CTLShift	= 20,
	kL2IShift	= 21,
	kL2DOShift	= 22,
	kL2RAMShift = 23,
	kL2CLKShift = 25,
	kL2SIZShift = 28,
	kL2PEShift	= 30,
	kL2EShift	= 31
} L2CRShifts;

enum {
	kL2IP	= 1 << kL2IPShift,
	kL2BYP	= 1 << kL2BYPShift,
	kL2DF	= 1 << kL2DFShift,
	kL2SL	= 1 << kL2SLShift,
	kL2OH	= 3 << kL2OHShift,
	kL2TS	= 1 << kL2TSShift,
	kL2WT	= 1 << kL2WTShift,
	kL2CTL	= 1 << kL2CTLShift,
	kL2I	= 1 << kL2IShift,
	kL2DO	= 1 << kL2DOShift,
	kL2RAM	= 3 << kL2RAMShift,
	kL2CLK	= 7 << kL2CLKShift,
	kL2SIZ	= 3 << kL2SIZShift,
	kL2PE	= 1 << kL2PEShift,
	kL2E	= 1 << kL2EShift
} L2CRMasks;

enum {
	kSize0		= 0,
	kSize256K	= 1,
	kSize512K	= 2,
	kSize1024K	= 3
} L2Sizes;

enum {
	kFlowThorugh			= 0,
	kPipelinedBurst			= 2,
	kPipelinedLateWrite		= 3
} L2Types;


// This is kind of backwards...  Index is the cache ratio times 2
// eg. PLLMode[(1:1) * 2] => PLLMode[2] => 1.
// the result goes in L2CLK
static int PLLModes[7] = {0, 0, 1, 2, 4, 5, 6};


// prototypes for all the local functions
unsigned int RunCacheTests(void);
int DetermineCacheType(v_u_long *buf, int cpu_speed);
int DetermineCacheSpeed(v_u_long *buf, int type, int cpu_speed);
int DetermineCacheSize(v_u_long *buf, int speed, int type, int cpu_speed);
void SetCacheMode(int size, int speed, int type, int cpu_speed);
int TestCache(v_u_long *buf, int size);


// InitBacksizeL2 is called in identify machine.
// If the L2 is already configured it just reports the size and type.
// If not it figures out what kind it is and sets it up.
// Also makes sure that PowerPC 750 Dynamic Power Management is on.
int InitBacksideL2(void)
{
  unsigned int L2CR, HID0, tmp;

  if (PROCESSOR_VERSION != PROCESSOR_VERSION_750) return 0;

  // Make sure dynamic power managment is on.
  mfspr(HID0, 1008);
  HID0 |= (1 << 20);  // DPM on
  mtspr(1008, HID0);

  // See if the L2 is already configured
  L2CR = mfl2cr();

  // If not try to set it up.
  if (mfl2cr() == 0) L2CR = RunCacheTests();

  // L2 is now fully configured so tell the system about it.

  // Backsize or none at all.
  if (L2CR != 0) powermac_machine_info.l2_cache_type = L2_CACHE_BACKSIDE;
  else powermac_machine_info.l2_cache_type = L2_CACHE_NONE;

  // Report the L2 Cache speed.
  switch ((L2CR & kL2CLK) >> kL2CLKShift) {
    case 1 : powermac_machine_info.l2_pll = 2; break;
    case 2 : powermac_machine_info.l2_pll = 3; break;
    case 4 : powermac_machine_info.l2_pll = 4; break;
    case 5 : powermac_machine_info.l2_pll = 5; break;
    case 6 : powermac_machine_info.l2_pll = 6; break;
    
    default : powermac_machine_info.l2_pll = -1; break;
  }

  // Report the L2 PLL mode.
  if (powermac_machine_info.l2_pll != -1)
    powermac_machine_info.l2_clock_rate_hz =
      (powermac_machine_info.cpu_clock_rate_hz * 2) /
      powermac_machine_info.l2_pll;

  // Report the L2 cache size.
  tmp = (L2CR & kL2SIZ) >> kL2SIZShift;
  if (tmp != 0) powermac_machine_info.l2_cache_size = 1 << (17 + tmp);

  return 1;
}


// RunCacheTests trys to determine the type, speed and size of the cache.
unsigned int RunCacheTests(void)
{
  unsigned int size, speed, type, cpu_speed;
  unsigned int HID0, L2CR, tmp, cnt, lbat, ubat;
  v_u_long *buf;

  // Must know the CPU speed for setting the Slow DLL bit.
  cpu_speed = powermac_machine_info.cpu_clock_rate_hz;
  
  // Set up dbat3 to map SR_COPYIN to the 31st MB of ram...
  // This will be undone before leaving the function.
  // Hard coding to use the 31st MB is a hack, but the ram
  // size is not easy to know right now.  Will be fixed later.

  // Invalidate dbat3
  ubat = 0;
  mtdbatu(3, ubat);
  sync();
  isync();

  ubat = (SR_COPYIN << 28) | 7 << 2 | 2;      // 0xe0000000, 1 MB, Vs
  lbat = (0x01f00000) | 3 << 3 | 2 ;          // 0x01f00000, wiMG, r/w

  // Seting the new BAT3 values.
  mtdbatl(3, lbat);
  mtdbatu(3, ubat);
  sync();
  isync();
  
  // buf gives us access to the temp memory block.
  buf = (v_u_long *)(SR_COPYIN << 28);

  // Flush the L1 Cache
  mfspr(HID0, 1008);
  tmp = HID0 | (1 << 6);  // DCFA on
  mtspr(1008, tmp);
  isync();
  
  for (cnt = 0; cnt < 128 * 8; cnt++) { // do a read from each cache entry
    tmp = buf[cnt * 8];                 // 8 longs per cache entry
  }
  
  // Turn the L1 off
  tmp = HID0 & ~(1 << 14); // DCE off
  mtspr(1008, tmp);
  isync();

  // Assume the L2 cache is bad.
  L2CR = 0;

  // This does all the tests.
  // Will ether set the copy of L2CR or fall through.
  type = DetermineCacheType(buf, cpu_speed);
  if (type != -1) {
    speed = DetermineCacheSpeed(buf, type, cpu_speed);
    if (speed != 0) {
      size = DetermineCacheSize(buf, speed, type, cpu_speed);
      if (size != kSize0) {
	// The L2 cache has been figured out.  Reset it so it can be used.
	SetCacheMode(size, speed, type, cpu_speed);
	L2CR = mfl2cr();
	L2CR &= ~(kL2DO | kL2TS);
      }
    }
  }

  // The copy L2CR now hold the best cache value or zero.

  // Set the L1 cache to its initial mode..
  mtspr(1008, HID0);
  isync();

  // Invalidate dbat3
  ubat = 0;
  mtdbatu(3, ubat);
  sync();
  isync();

  // Turn the L2 on if it check out.
  if (L2CR != 0) {
    L2CR |= kL2E;
    mtl2cr(L2CR);
    isync();
  }

  // Return with all caches enabled and ready for action.

  return L2CR;
}


// DetermineCacheType trys to set the cache to each of the type and looks
// to see which one works.  Only one will work so it returns when it finds
// it.  If none work, there must not be a cache attached.
// Assumes that the caches is as slow as possibe (3:1) and as small as
// possible (256k).
int DetermineCacheType(v_u_long *buf, int cpu_speed)
{
  int result;

  // Test for Flow-through
  SetCacheMode(kSize256K, 6, kFlowThorugh, cpu_speed);
  result = TestCache(buf, kSize256K);
  if (result != 0) return kFlowThorugh;

  // Test for Pipelined Burst
  SetCacheMode(kSize256K, 6, kPipelinedBurst, cpu_speed);
  result = TestCache(buf, kSize256K);
  if (result != 0) return kPipelinedBurst;

  // Test for Pipelined Late-write
  SetCacheMode(kSize256K, 6, kPipelinedLateWrite, cpu_speed);
  result = TestCache(buf, kSize256K);
  if (result != 0) return kPipelinedLateWrite;

  return -1;
}


// DetermineCacheSpeed trys to find out how fast the cache can go.
// Starts at the fastest (1:1) and works its way down to the slowest (3:1).
// Returns on the first success or return 0.
// There is a remote chance that a speed will be selected that works at startup
// but not after the machine has warmed up.  Well, I hope it is remote.
// Later a backdoor will be added to let the use overide this value.
int DetermineCacheSpeed(v_u_long *buf, int type, int cpu_speed)
{
  int speed, result;

  for (speed = 2; speed <= 6; speed++) {
    SetCacheMode(kSize256K, speed, type, cpu_speed);
    result = TestCache(buf, kSize256K);
    if (result) return speed;
  }
  
  return 0;
}


// DetermineCacheSize figures out the size of the cache by trying to validate
// each of the sizes with the selected type and speed.  Pretty simple.
int DetermineCacheSize(v_u_long *buf, int speed, int type, int cpu_speed)
{
  int size, result;

  for (size = kSize1024K; size != kSize0; size--) {
    SetCacheMode(size, speed, type, cpu_speed);
    result = TestCache(buf, size);
    if (result) return size;
  }

  return kSize0;
}


// SetCacheMode prepares the cache for a test.  Sets the new mode,
// syncs the DLL, and invalidates the cache.
void SetCacheMode(int size, int speed, int type, int cpu_speed)
{
  unsigned int	L2CR;

  // Set the size.
  L2CR = size << kL2SIZShift;

  // Set the Speed.
  L2CR |= PLLModes[speed] << kL2CLKShift;

  // Set the slow bit if needed.
  if (((cpu_speed * 2) / speed) < 100000000)
    L2CR |= kL2SL;
    
  // Set the sram type.
  L2CR |= type << kL2RAMShift;

  // Set differential clock and 1.0ns OH if late-write.
  if (type == kPipelinedLateWrite)
    L2CR |= kL2DF | (1 << kL2OHShift);

  // The the cache in the data only test mode.
  L2CR |= kL2DO | kL2TS;

  // Set the new mode
  mtl2cr(L2CR);
  isync();

  // Do a global invalidate to clear the cache and sync the DLL.
  L2CR |= kL2I;
  sync();
  mtl2cr(L2CR);
  isync();

  // Wait for the global invalidate to finish.
  do {
    L2CR = mfl2cr();
  } while(L2CR & kL2IP);

  // Put the cache back into normal mode.
  L2CR &= ~kL2I;
  mtl2cr(L2CR);
  isync();
}

// TestCache try to see if that cache works in the selected mode.
// l1 & l2 caches are dissabled.  buf points to 1MB of ram.
// return 0 if bad, 1 if good.
// The assembled output should be examined to make sure there is no
// "extra" memory accesses durring the test.
int TestCache(v_u_long *buf, int size)
{
  int cnt, lsize, tmp;
  unsigned int L2CR, HID0;

  // calculate the number of longs
  lsize = 1 << (15 + size);     // 17 + size - 2...

  // Init the memory buffer to buf[addr] = lsize - addr
  for (cnt = 0; cnt < lsize; cnt++) buf[cnt] = lsize - cnt;

  // Turn the L1 cache on
  mfspr(HID0, 1008);
  tmp = HID0 | (1 << 14); // DCE on
  mtspr(1008, tmp);
  isync();

  // Turn on the L2 cache.
  L2CR = mfl2cr();
  L2CR |= kL2E;
  mtl2cr(L2CR);
  isync();

  // Fill the L2 with the test values buf[addr] = addr
  for (cnt = 0; cnt < lsize; cnt++) {
    buf[cnt] = cnt;
    if ((cnt & 7) == 7) dcbf((unsigned long)&buf[cnt]);
  }

  eieio();
  sync();

  // Turn the L1 cache off
  mtspr(1008, HID0);
  isync();

  // Read the cache to see if the correct test values are there.
  for (cnt = 0; cnt < lsize; cnt++) {
    if (buf[cnt] != cnt) {
      // It did not read the expected value... Turn off the l2 cache and
      // return 0;
      L2CR = 0;
      mtl2cr(L2CR);
      isync();

      return 0;
    }
  }
  
  // If it got here it is good.  Turn off the L2 cache and return.
  L2CR = mfl2cr();
  L2CR &= ~kL2E;
  mtl2cr(L2CR);
  isync();

  return 1;
}

