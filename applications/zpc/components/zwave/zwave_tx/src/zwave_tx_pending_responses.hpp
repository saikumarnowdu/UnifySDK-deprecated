/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

/**
 * @defgroup zwave_tx_pending_responses Z-Wave Tx pending responses
 * @ingroup zwave_tx
 * @brief Tracks singlecast frames awaiting application responses per NodeID.
 *
 * Unlike the global TX back-off, pending responses only block further
 * transmissions to the same destination NodeID.
 *
 * @{
 */

#ifndef ZWAVE_TX_PENDING_RESPONSES_HPP
#define ZWAVE_TX_PENDING_RESPONSES_HPP

#include "sl_status.h"
#include "zwave_controller_types.h"
#include "zwave_tx_definitions.h"

#include <cstdint>
#include <algorithm>

#ifndef ZWAVE_TX_INCOMING_FRAMES_BUFFER_SIZE
#define ZWAVE_TX_INCOMING_FRAMES_BUFFER_SIZE 10
#endif

using zwave_tx_pending_response_item_t = struct zwave_tx_pending_response_item {
  zwave_tx_session_id_t session_id;
  zwave_node_id_t node_id;
  uint32_t deadline;
};

/**
 * @brief Fixed-size list of frames waiting for a response from a NodeID.
 */
class zwave_tx_pending_responses
{
  public:
  /**
   * @brief Register a session that is waiting for responses from a NodeID.
   *
   * @returns SL_STATUS_OK on success, SL_STATUS_FULL if the list is full.
   */
  sl_status_t add(zwave_tx_session_id_t session_id,
                  zwave_node_id_t node_id,
                  uint32_t deadline)
  {
    if (is_session_pending(session_id)) {
      return SL_STATUS_OK;
    }

    for (size_t i = 0; i < count; i++) {
      if (items[i].session_id == session_id) {
        items[i].node_id   = node_id;
        items[i].deadline  = deadline;
        return SL_STATUS_OK;
      }
    }

    if (count >= max_size()) {
      return SL_STATUS_FULL;
    }

    items[count].session_id = session_id;
    items[count].node_id    = node_id;
    items[count].deadline   = deadline;
    count++;
    return SL_STATUS_OK;
  }

  sl_status_t remove(zwave_tx_session_id_t session_id)
  {
    for (size_t i = 0; i < count; i++) {
      if (items[i].session_id == session_id) {
        items[i] = items[count - 1];
        count--;
        return SL_STATUS_OK;
      }
    }
    return SL_STATUS_NOT_FOUND;
  }

  bool is_session_pending(zwave_tx_session_id_t session_id) const
  {
    for (size_t i = 0; i < count; i++) {
      if (items[i].session_id == session_id) {
        return true;
      }
    }
    return false;
  }

  bool is_node_waiting(zwave_node_id_t node_id) const
  {
    for (size_t i = 0; i < count; i++) {
      if (items[i].node_id == node_id) {
        return true;
      }
    }
    return false;
  }

  uint32_t earliest_deadline() const
  {
    uint32_t earliest = 0;
    for (size_t i = 0; i < count; i++) {
      if (earliest == 0 || items[i].deadline < earliest) {
        earliest = items[i].deadline;
      }
    }
    return earliest;
  }

  void clear()
  {
    count = 0;
  }

  bool empty() const
  {
    return count == 0;
  }

  size_t size() const
  {
    return count;
  }

  static constexpr size_t max_size()
  {
    return ZWAVE_TX_INCOMING_FRAMES_BUFFER_SIZE;
  }

  /**
   * @brief Invokes @p visitor for each pending item whose deadline has passed.
   *
   * @returns Number of expired items visited.
   */
  template<typename Visitor>
  size_t for_each_expired(uint32_t now, Visitor visitor)
  {
    size_t expired_count = 0;
    for (size_t i = 0; i < count;) {
      if (items[i].deadline <= now) {
        visitor(items[i]);
        items[i] = items[count - 1];
        count--;
        expired_count++;
      } else {
        i++;
      }
    }
    return expired_count;
  }

  /**
   * @brief Invokes @p visitor for each pending item matching @p node_id.
   */
  template<typename Visitor>
  void for_each_on_node(zwave_node_id_t node_id, Visitor visitor) const
  {
    for (size_t i = 0; i < count; i++) {
      if (items[i].node_id == node_id) {
        visitor(items[i]);
      }
    }
  }

  private:
  zwave_tx_pending_response_item_t items[ZWAVE_TX_INCOMING_FRAMES_BUFFER_SIZE]
    = {};
  size_t count = 0;
};

#endif  // ZWAVE_TX_PENDING_RESPONSES_HPP
/** @} end zwave_tx_pending_responses */
