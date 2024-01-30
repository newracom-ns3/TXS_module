/*
 * Copyright (c) 2024 Newracom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2016
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Contributed by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *                Sébastien Deronne <sebastien.deronne@gmail.com>
 *
 * This code has been based on ns-3 (wifi/model/wifi-mac.{cc,h})
 *                                  (wifi/model/wifi-mac-helper.{cc,h})
 *
 * Author: Seungmin Lee <sm.lee@newratek.com>
 *         Changmin Lee <cm.lee@newratek.com>
 */

#ifndef TXS_WIFI_MAC_HELPER_H
#define TXS_WIFI_MAC_HELPER_H

#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-mac.h"

namespace ns3
{

class WifiMac;
class WifiNetDevice;

class TxsWifiMac : public WifiMac
{
  public:
    TxsWifiMac();
    ~TxsWifiMac();
    void SetupFrameExchangeManager(WifiStandard standard,
                                   WifiMac::LinkEntity& link,
                                   bool txsSupported);
    void FemConfigureStandard(WifiStandard standard, bool txsSupported);
};

class TxsWifiMacHelper : public WifiMacHelper
{
  public:
    TxsWifiMacHelper(bool enable = false);
    ~TxsWifiMacHelper();

    Ptr<WifiMac> Create(Ptr<WifiNetDevice> device, WifiStandard standard) const override;

  private:
    bool m_txsSupported;
};

} // namespace ns3

#endif