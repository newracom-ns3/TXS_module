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

#ifndef TXS_MULTI_USER_SCHEDULER_H
#define TXS_MULTI_USER_SCHEDULER_H

#include "ns3/rr-multi-user-scheduler.h"

// Add a doxygen group for this module.
// If you have more than one file, this should be in only one of them.
/**
 * \defgroup txs-module Description of the txs-module
 */

namespace ns3
{

class TxsMultiUserScheduler : public RrMultiUserScheduler
{
  public:
    static TypeId GetTypeId();
    TxsMultiUserScheduler();
    ~TxsMultiUserScheduler();
    Mac48Address& GetFirstAssocStaList();
    bool CheckLastTxIsDlMu();
    void NotifyAccessGranted(u_int8_t linkId);

  protected:
    void DoDispose() override;

  private:
    void AccessReqTimeout();
};

} // namespace ns3

#endif /* TXS_MULTI_USER_SCHEDULER_H */
