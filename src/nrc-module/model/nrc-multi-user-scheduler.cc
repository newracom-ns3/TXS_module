#include "nrc-multi-user-scheduler.h"

#include "ns3/log.h"

// #include "ns3/simulator.h"
#include "nrc-frame-exchange-manager.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrcMultiUserScheduler");
NS_OBJECT_ENSURE_REGISTERED(NrcMultiUserScheduler);

TypeId
NrcMultiUserScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NrcMultiUserScheduler")
                            .SetParent<RrMultiUserScheduler>()
                            .AddConstructor<NrcMultiUserScheduler>()
                            .SetGroupName("Wifi");
    return tid;
}

NrcMultiUserScheduler::NrcMultiUserScheduler()
{
    NS_LOG_FUNCTION(this);
}

NrcMultiUserScheduler::~NrcMultiUserScheduler()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
NrcMultiUserScheduler::DoDispose()
{
    NS_LOG_FUNCTION(this);
    RrMultiUserScheduler::DoDispose();
}

Mac48Address&
NrcMultiUserScheduler::GetFirstAssocStaList()
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
NrcMultiUserScheduler::CheckLastTxIsDlMu()
{
    return GetLastTxFormat(m_linkId) == DL_MU_TX;
}

void
NrcMultiUserScheduler::NotifyAccessGranted(u_int8_t linkId)
{
    if (m_accessReqTimer.IsRunning() && m_restartTimerUponAccess)
    {
        // restart access timer
        m_accessReqTimer.Cancel();
        if (m_accessReqInterval.IsStrictlyPositive())
        {
            m_accessReqTimer = Simulator::Schedule(m_accessReqInterval,
                                                   &NrcMultiUserScheduler::AccessReqTimeout,
                                                   this);
        }
    }

    m_lastTxInfo[m_linkId].lastTxFormat = SU_TX;
}

void
NrcMultiUserScheduler::AccessReqTimeout()
{
    MultiUserScheduler::AccessReqTimeout();
}

} // namespace ns3
