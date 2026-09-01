/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
// Includes from this component
#include "zpc_attribute_resolver_callbacks.h"
#include "zpc_attribute_resolver_fixt.h"
#include "zpc_attribute_resolver_group.h"
#include "zpc_attribute_resolver_send.h"

// Unify Components includes
#include "attribute_resolver.h"

// ZPC components
#include "zpc_attribute_store_network_helper.h"

static uintptr_t
  zpc_attribute_resolver_get_parallel_lane(attribute_store_node_t node)
{
  zwave_node_id_t node_id         = 0;
  zwave_endpoint_id_t endpoint_id = 0;

  if (SL_STATUS_OK
      != attribute_store_network_helper_get_zwave_ids_from_node(node,
                                                                &node_id,
                                                                &endpoint_id)) {
    return 0;
  }

  return (uintptr_t)node_id;
}

sl_status_t zpc_attribute_resolver_init()
{
  sl_status_t init_status = SL_STATUS_OK;
  // Initialize our group handling component.
  init_status |= zpc_attribute_resolver_group_init();

  attribute_resolver_config_t attribute_resolver_config
    = {.send_init         = &attribute_resolver_send_init,
       .send              = &attribute_resolver_send,
       .abort             = &attribute_resolver_abort_pending_resolution,
       .get_parallel_lane = &zpc_attribute_resolver_get_parallel_lane,
       // Minimal timespan before retrying a get
       .get_retry_timeout = 3000,
       // Number of times to retry sending a get
       .get_retry_count   = 5};
  init_status |= attribute_resolver_init(attribute_resolver_config);
  return init_status;
}

int zpc_attribute_resolver_teardown()
{
  zpc_attribute_resolver_group_teardown();
  return attribute_resolver_teardown();
}