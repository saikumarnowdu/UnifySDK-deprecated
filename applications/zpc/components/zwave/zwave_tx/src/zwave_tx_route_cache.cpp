/******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "zwave_tx_route_cache.h"

// Generic includes
#include <array>
#include <algorithm>

// Unify shared components
#include "sl_log.h"
constexpr char LOG_TAG[] = "zwave_tx_route_cache";

using route_cache_data_t = struct {
  zwave_node_id_t node_id;
  uint8_t number_of_repeaters;
  int8_t ack_rssi;
  last_route_speed_t last_route_speed;
  bool used_beam;
  bool valid;
};

namespace
{
std::array<route_cache_data_t, ZWAVE_TX_ROUTE_CACHE_BUFFER_SIZE> route_cache;

route_cache_data_t *find_entry(zwave_node_id_t node_id)
{
  if (node_id == 0) {
    return nullptr;
  }
  for (auto &cache: route_cache) {
    if (cache.node_id == node_id) {
      return &cache;
    }
  }
  return nullptr;
}

route_cache_data_t *find_or_allocate(zwave_node_id_t node_id)
{
  route_cache_data_t *existing = find_entry(node_id);
  if (existing != nullptr) {
    return existing;
  }
  for (auto &cache: route_cache) {
    if (cache.node_id == 0) {
      cache.node_id = node_id;
      return &cache;
    }
  }
  return nullptr;
}

void clear_entry(route_cache_data_t &cache)
{
  cache = {0, 0, RSSI_NOT_AVAILABLE, UNKNOWN_SPEED, false, false};
}
}  // namespace

void zwave_tx_route_cache_init()
{
  route_cache.fill({0, 0, RSSI_NOT_AVAILABLE, UNKNOWN_SPEED, false, false});
}

void zwave_tx_route_cache_set_number_of_repeaters(
  zwave_node_id_t destination_node_id, uint8_t number_of_repeaters)
{
  route_cache_data_t *cache = find_entry(destination_node_id);
  if (cache != nullptr) {
    cache->number_of_repeaters = number_of_repeaters;
    cache->valid               = true;
    if (0 == number_of_repeaters) {
      clear_entry(*cache);
    }
    return;
  }

  if (number_of_repeaters == 0) {
    return;
  }
  cache = find_or_allocate(destination_node_id);
  if (cache == nullptr) {
    sl_log_debug(LOG_TAG,
                 "No more queue space to save routed destinations. "
                 "Ignoring");
    return;
  }
  cache->number_of_repeaters = number_of_repeaters;
  cache->ack_rssi            = RSSI_NOT_AVAILABLE;
  cache->last_route_speed    = UNKNOWN_SPEED;
  cache->used_beam           = false;
  cache->valid               = true;
}

uint8_t zwave_tx_route_cache_get_number_of_repeaters(
  zwave_node_id_t destination_node_id)
{
  const route_cache_data_t *cache = find_entry(destination_node_id);
  if (cache != nullptr && cache->valid) {
    return cache->number_of_repeaters;
  }
  return 0;
}

void zwave_tx_route_cache_update_from_tx_report(
  zwave_node_id_t destination_node_id, const zwapi_tx_report_t *tx_report)
{
  if (tx_report == nullptr || destination_node_id == 0) {
    return;
  }

  route_cache_data_t *cache = find_or_allocate(destination_node_id);
  if (cache == nullptr) {
    sl_log_debug(LOG_TAG,
                 "No more cache space to save TX metrics for NodeID %d.",
                 destination_node_id);
    return;
  }

  cache->number_of_repeaters = tx_report->number_of_repeaters;
  cache->ack_rssi            = tx_report->ack_rssi;
  cache->last_route_speed    = tx_report->last_route_speed;
  cache->used_beam = (tx_report->beam_250ms || tx_report->beam_1000ms);
  cache->valid     = true;
}

uint32_t zwave_tx_route_cache_link_score(zwave_node_id_t destination_node_id)
{
  const route_cache_data_t *cache = find_entry(destination_node_id);
  if (cache == nullptr || cache->valid == false) {
    // Unknown dest: mid-range so first contact is neither starved nor first.
    return 360;
  }

  uint32_t score = 0;

  // Listening / no-beam destinations (AL routers) before FL/NL beams.
  if (cache->used_beam == false) {
    score += 400;
  }

  // Direct range before multi-hop (routed) destinations.
  const uint8_t hops = std::min<uint8_t>(cache->number_of_repeaters, 4);
  score += static_cast<uint32_t>(4 - hops) * 50u;

  switch (cache->last_route_speed) {
    case ZWAVE_LONG_RANGE_100_KBITS_S:
    case ZWAVE_100_KBITS_S:
      score += 100;
      break;
    case ZWAVE_40_KBITS_S:
      score += 50;
      break;
    case ZWAVE_9_6_KBITS_S:
      score += 0;
      break;
    case UNKNOWN_SPEED:
    default:
      score += 40;
      break;
  }

  if (cache->ack_rssi != RSSI_NOT_AVAILABLE) {
    int rssi_offset = static_cast<int>(cache->ack_rssi) + 100;
    if (rssi_offset < 0) {
      rssi_offset = 0;
    }
    if (rssi_offset > 80) {
      rssi_offset = 80;
    }
    score += static_cast<uint32_t>(rssi_offset);
  } else {
    score += 20;
  }

  return score;
}
