/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Hal.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
SetProcessPriority(int aPid,
                   ProcessPriority aPriority,
                   ProcessCPUPriority aCPUPriority,
                   uint32_t aBackgroundLRU)
{
  HAL_LOG(("FallbackProcessPriority - SetProcessPriority(%d, %s, %u)\n",
           aPid, ProcessPriorityToString(aPriority, aCPUPriority),
           aBackgroundLRU));
}

} // hal_impl
} // namespace mozilla
