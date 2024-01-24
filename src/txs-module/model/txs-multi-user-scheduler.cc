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
 *
 * Author: Seungmin Lee <sm.lee@newratek.com>

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
