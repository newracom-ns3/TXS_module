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
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
 * Contributed by Stefano Avallone <stavallo@unina.it>
 *
 * This code has been based on ns-3 (wifi/model/he/rr-multi-user-scheduler.{cc,h})
 * Author: Seungmin Lee <sm.lee@newratek.com>
 *         Changmin Lee <cm.lee@newratek.com>
 */

#include "txs-multi-user-scheduler.h"

#include "ns3/log.h"

// #include "ns3/simulator.h"
#include "txs-frame-exchange-manager.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TxsMultiUserScheduler");
NS_OBJECT_ENSURE_REGISTERED(TxsMultiUserScheduler);

TypeId
TxsMultiUserScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TxsMultiUserScheduler")
                            .SetParent<RrMultiUserScheduler>()
                            .AddConstructor<TxsMultiUserScheduler>()
                            .SetGroupName("Wifi");
    return tid;
}

TxsMultiUserScheduler::TxsMultiUserScheduler()
{
    NS_LOG_FUNCTION(this);
}

TxsMultiUserScheduler::~TxsMultiUserScheduler()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
TxsMultiUserScheduler::DoDispose()
{
    NS_LOG_FUNCTION(this);
    RrMultiUserScheduler::DoDispose();
}

Mac48Address&
TxsMultiUserScheduler::GetFirstAssocStaList()
{
    auto it = m_staListUl.begin();
    Mac48Address ulSta("00:00:00:00:00:02");

    while (it != m_staListUl.end())
    {
        if (it->address == ulSta)
        {
            break;
        }
        ++it;
    }
    return it->address;
}

bool
TxsMultiUserScheduler::CheckLastTxIsDlMu()
{
    return GetLastTxFormat(m_linkId) == DL_MU_TX;
}

void
TxsMultiUserScheduler::NotifyAccessGranted(u_int8_t linkId)
{
    if (m_accessReqTimer.IsRunning() && m_restartTimerUponAccess)
    {
        // restart access timer
        m_accessReqTimer.Cancel();
        if (m_accessReqInterval.IsStrictlyPositive())
        {
            m_accessReqTimer = Simulator::Schedule(m_accessReqInterval,
                                                   &TxsMultiUserScheduler::AccessReqTimeout,
                                                   this);
        }
    }

    m_lastTxInfo[m_linkId].lastTxFormat = SU_TX;
}

void
TxsMultiUserScheduler::AccessReqTimeout()
{
    MultiUserScheduler::AccessReqTimeout();
}

} // namespace ns3
